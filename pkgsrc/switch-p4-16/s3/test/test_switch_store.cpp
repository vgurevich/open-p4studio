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
#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <queue>

#include "bf_switch/bf_switch_types.h"
#include "gen-model/test_model.h"
#include "s3/attribute.h"
#include "s3/attribute_util.h"
#include "s3/switch_store.h"
#include "s3/factory.h"
#include "s3/smi.h"
#include "../log.h"

using namespace smi;

static ModelInfo *model_info = NULL;
static switch_object_type_t test_object_1;
const ObjectInfo *test_object_1_info;
static switch_object_type_t test_object_2;
const ObjectInfo *test_object_2_info;
static switch_object_type_t test_object_3;
const ObjectInfo *test_object_3_info;
static switch_object_type_t test_object_4;
const ObjectInfo *test_object_4_info;
static switch_object_type_t test_key_1;
const ObjectInfo *test_key_1_info;
static switch_object_type_t test_key_2;
const ObjectInfo *test_key_2_info;
static switch_object_type_t test_key_3;
const ObjectInfo *test_key_3_info;
static switch_object_type_t test_object_7;
const ObjectInfo *test_object_7_info;
static switch_object_type_t test_object_invalid;
const ObjectInfo *test_object_trigger_info;
static switch_object_type_t test_object_trigger_ot;

const ObjectInfo *test_object_referred_info;
static switch_object_type_t test_object_referred_ot;
const ObjectInfo *test_object_referrer_1_info;
static switch_object_type_t test_object_referrer_1_ot;
const ObjectInfo *test_object_referrer_2_info;
static switch_object_type_t test_object_referrer_2_ot;
const ObjectInfo *test_object_referrer_3_info;
static switch_object_type_t test_object_referrer_3_ot;
const ObjectInfo *test_object_referred_auto_1_info;
static switch_object_type_t test_object_referred_auto_1_ot;
const ObjectInfo *test_object_referred_auto_2_info;
static switch_object_type_t test_object_referred_auto_2_ot;

const ObjectInfo *test_object_group_membership_info;
static switch_object_type_t test_object_group_membership_ot;
static switch_attr_id_t test_attribute_list_oid;
const ObjectInfo *test_membership_auto_object_info;
static switch_object_type_t test_membership_auto_object_ot;

static switch_attr_id_t key_1_attr;
static switch_attr_id_t key_2_attr;
static switch_attr_id_t key_3_attr;
static switch_attr_id_t obj3_key1;
static switch_attr_id_t obj3_key2;
static switch_attr_id_t obj3_key3;
static switch_attr_id_t obj3_key4;

static switch_attr_id_t obj_3_key_prefix_attr;
static switch_attr_id_t obj_4_key_64_attr;
static switch_attr_id_t obj_4_key_mac_attr;
static switch_attr_id_t obj_7_key_1_attr;
static switch_attr_id_t obj_7_key_2_attr;

static switch_attr_id_t test_trig_obj_random_seed_before_1;
static switch_attr_id_t test_trig_obj_random_seed_before_2;
static switch_attr_id_t test_trig_obj_random_seed_before_3;
static switch_attr_id_t test_trig_obj_random_seed_after_1;
static switch_attr_id_t test_trig_obj_random_seed_after_2;
static switch_attr_id_t test_trig_obj_random_seed_after_3;
static switch_attr_id_t before_create_trigger_1_fail;
static switch_attr_id_t before_create_trigger_2_fail;
static switch_attr_id_t before_create_trigger_3_fail;
static switch_attr_id_t after_create_trigger_1_fail;
static switch_attr_id_t after_create_trigger_2_fail;
static switch_attr_id_t after_create_trigger_3_fail;

static uint8_t test_object_referred_auto_1_create_update_count,
    test_object_referred_auto_2_create_update_count,
    test_object_referred_auto_1_delete_count,
    test_object_referred_auto_2_delete_count;

static uint8_t test_auto_6_var;
static uint8_t test_mem_count;

class test_mem_auto_obj_1 : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_TEST_MEMBERSHIP_AUTO_OBJECT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_MEMBERSHIP_AUTO_OBJECT_ATTR_PARENT_HANDLE;

 public:
  test_mem_auto_obj_1(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }

  switch_status_t create_update() {
    test_mem_count++;
    return auto_object::create_update();
  }

  switch_status_t del() {
    test_mem_count -= 7;
    return auto_object::del();
  }
};

class test_object_referred_auto_1 : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_TEST_OBJECT_REFERRED_AUTO_1;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_OBJECT_REFERRED_AUTO_1_ATTR_PARENT_HANDLE;

 public:
  test_object_referred_auto_1(const switch_object_id_t parent,
                              switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    test_object_referred_auto_1_create_update_count++;
    status = auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    test_object_referred_auto_1_delete_count++;
    status = auto_object::del();
    return status;
  }
};

class test_object_referred_auto_2 : public auto_object {
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_TEST_OBJECT_REFERRED_AUTO_2;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_OBJECT_REFERRED_AUTO_2_ATTR_PARENT_HANDLE;

 public:
  test_object_referred_auto_2(const switch_object_id_t parent,
                              switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    test_object_referred_auto_2_create_update_count++;
    status = auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    test_object_referred_auto_2_delete_count++;
    status = auto_object::del();
    return status;
  }
};

class test_auto_6 : public auto_object {
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_6;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_6_ATTR_PARENT_HANDLE;

 public:
  test_auto_6(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }

  switch_status_t create_update() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    test_auto_6_var++;
    status = auto_object::create_update();
    return status;
  }

  switch_status_t del() {
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    test_auto_6_var--;
    status = auto_object::del();
    return status;
  }
};

void init_objects() {
  test_object_1_info = model_info->get_object_info_from_name("test_object_1");
  test_object_1 = test_object_1_info->object_type;
  assert(test_object_1 != 0);
  test_object_2_info = model_info->get_object_info_from_name("test_object_2");
  test_object_2 = test_object_2_info->object_type;
  assert(test_object_2 != 0);
  test_object_3_info = model_info->get_object_info_from_name("test_object_3");
  test_object_3 = test_object_3_info->object_type;
  assert(test_object_3 != 0);
  test_object_4_info = model_info->get_object_info_from_name("test_object_4");
  test_object_4 = test_object_4_info->object_type;
  assert(test_object_4 != 0);
  obj_4_key_64_attr = test_object_4_info->get_attr_id_from_name("test_uint64");
  assert(obj_4_key_64_attr != 0);
  obj_4_key_mac_attr = test_object_4_info->get_attr_id_from_name("test_mac");
  assert(obj_4_key_mac_attr != 0);
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
  obj3_key1 = test_object_3_info->get_attr_id_from_name("test_key1");
  assert(obj3_key1 != 0);
  obj3_key2 = test_object_3_info->get_attr_id_from_name("test_key2");
  assert(obj3_key2 != 0);
  obj3_key3 = test_object_3_info->get_attr_id_from_name("test_key3");
  assert(obj3_key3 != 0);
  obj3_key4 = test_object_3_info->get_attr_id_from_name("test_key4");
  assert(obj3_key4 != 0);
  test_object_7_info = model_info->get_object_info_from_name("test_object_7");
  test_object_7 = test_object_7_info->object_type;
  assert(test_object_7 != 0);
  obj_7_key_1_attr = test_object_7_info->get_attr_id_from_name("test_uint8");
  assert(obj_7_key_1_attr != 0);
  obj_7_key_2_attr = test_object_7_info->get_attr_id_from_name("test_uint64");
  assert(obj_7_key_2_attr != 0);
  test_object_invalid = 100;

  test_object_referred_info =
      model_info->get_object_info_from_name("test_object_referred");
  test_object_referred_ot = test_object_referred_info->object_type;
  assert(test_object_referred_ot != 0);
  test_object_referrer_1_info =
      model_info->get_object_info_from_name("test_object_referrer_1");
  test_object_referrer_1_ot = test_object_referrer_1_info->object_type;
  assert(test_object_referrer_1_ot != 0);
  test_object_referrer_2_info =
      model_info->get_object_info_from_name("test_object_referrer_2");
  test_object_referrer_2_ot = test_object_referrer_2_info->object_type;
  assert(test_object_referrer_2_ot != 0);
  test_object_referrer_3_info =
      model_info->get_object_info_from_name("test_object_referrer_3");
  test_object_referrer_3_ot = test_object_referrer_3_info->object_type;
  assert(test_object_referrer_3_ot != 0);
  test_object_referred_auto_1_info =
      model_info->get_object_info_from_name("test_object_referred_auto_1");
  test_object_referred_auto_1_ot =
      test_object_referred_auto_1_info->object_type;
  assert(test_object_referred_auto_1_ot != 0);
  test_object_referred_auto_2_info =
      model_info->get_object_info_from_name("test_object_referred_auto_2");
  test_object_referred_auto_2_ot =
      test_object_referred_auto_2_info->object_type;
  assert(test_object_referred_auto_2_ot != 0);

  test_object_group_membership_info =
      model_info->get_object_info_from_name("test_object_group_membership");
  test_object_group_membership_ot =
      test_object_group_membership_info->object_type;
  assert(test_object_group_membership_ot != 0);
  test_attribute_list_oid =
      test_object_group_membership_info->get_attr_id_from_name(
          "test_attribute_list_oid");
  assert(test_attribute_list_oid != 0);
  test_membership_auto_object_info =
      model_info->get_object_info_from_name("test_membership_auto_object");
  test_membership_auto_object_ot =
      test_membership_auto_object_info->object_type;
  assert(test_membership_auto_object_ot != 0);

  test_object_trigger_info =
      model_info->get_object_info_from_name("test_trigger_object");
  test_object_trigger_ot = test_object_trigger_info->object_type;
  assert(test_object_trigger_ot != 0);

  test_trig_obj_random_seed_before_1 =
      test_object_trigger_info->get_attr_id_from_name("random_seed_before_1");
  assert(test_trig_obj_random_seed_before_1 != 0);

  test_trig_obj_random_seed_before_2 =
      test_object_trigger_info->get_attr_id_from_name("random_seed_before_2");
  assert(test_trig_obj_random_seed_before_2 != 0);

  test_trig_obj_random_seed_before_3 =
      test_object_trigger_info->get_attr_id_from_name("random_seed_before_3");
  assert(test_trig_obj_random_seed_before_3 != 0);

  test_trig_obj_random_seed_after_1 =
      test_object_trigger_info->get_attr_id_from_name("random_seed_after_1");
  assert(test_trig_obj_random_seed_after_1 != 0);

  test_trig_obj_random_seed_after_2 =
      test_object_trigger_info->get_attr_id_from_name("random_seed_after_2");
  assert(test_trig_obj_random_seed_after_2 != 0);

  test_trig_obj_random_seed_after_3 =
      test_object_trigger_info->get_attr_id_from_name("random_seed_after_3");
  assert(test_trig_obj_random_seed_after_3 != 0);

  before_create_trigger_1_fail =
      test_object_trigger_info->get_attr_id_from_name(
          "before_create_trigger_1_fail");
  assert(before_create_trigger_1_fail != 0);
  before_create_trigger_2_fail =
      test_object_trigger_info->get_attr_id_from_name(
          "before_create_trigger_2_fail");
  assert(before_create_trigger_2_fail != 0);
  before_create_trigger_3_fail =
      test_object_trigger_info->get_attr_id_from_name(
          "before_create_trigger_3_fail");
  assert(before_create_trigger_3_fail != 0);

  after_create_trigger_1_fail = test_object_trigger_info->get_attr_id_from_name(
      "after_create_trigger_1_fail");
  assert(after_create_trigger_1_fail != 0);
  after_create_trigger_2_fail = test_object_trigger_info->get_attr_id_from_name(
      "after_create_trigger_2_fail");
  assert(after_create_trigger_2_fail != 0);
  after_create_trigger_3_fail = test_object_trigger_info->get_attr_id_from_name(
      "after_create_trigger_3_fail");
  assert(after_create_trigger_3_fail != 0);
}

void test_object_basic() {
  std::cout << "**** Tesing basic objects ****" << std::endl;
  switch_status_t status;
  switch_object_id_t empty_oid = {};
  switch_attr_id_t invalid_attr_id = 0x3fff;

  switch_attr_id_t attr_id =
      test_object_1_info->get_attr_id_from_name("test_attribute_uint64");
  assert(attr_id != 0);

  uint64_t val = 0;
  const std::set<attr_w> attrs{attr_w(attr_id, val)};
  switch_object_id_t oid = {};
  /* create */
  status = switch_store::object_create(test_object_1, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_create(test_object_invalid, attrs, oid);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  val++;
  /* set */
  status = switch_store::v_set(oid, attr_id, val);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::v_set(empty_oid, attr_id, val);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::v_set(empty_oid, invalid_attr_id, val);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  uint64_t val_out = 0;
  switch_object_id_t oid_val_out;

  /* get */
  status = switch_store::v_get(oid, attr_id, val_out);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(val_out == val);

  status = switch_store::v_get(empty_oid, attr_id, val_out);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::v_get(empty_oid, invalid_attr_id, val_out);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  /* bad type get */
  status = switch_store::v_get(oid, attr_id, oid_val_out);
  assert(status != SWITCH_STATUS_SUCCESS);

  std::string name = switch_store::object_name_get_from_object(oid);

  /* get auto populated default value */
  attr_id = test_object_1_info->get_attr_id_from_name("test_attribute_oid");
  assert(attr_id != 0);
  status = switch_store::v_get(oid, attr_id, oid_val_out);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(oid_val_out == 0);

  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  /* create key attribute objects */
  uint64_t key1 = 1;
  uint64_t key2 = 2;
  uint64_t key22 = 22;
  uint64_t key3 = 3;
  const std::set<attr_w> key_1_attrs{attr_w(key_1_attr, key1)};
  const std::set<attr_w> key_2_attrs{attr_w(key_2_attr, key2)};
  const std::set<attr_w> key_22_attrs{attr_w(key_2_attr, key22)};
  const std::set<attr_w> key_3_attrs{attr_w(key_3_attr, key3)};

  switch_object_id_t key_1_oid = {}, key_2_oid = {}, key_3_oid = {},
                     key_22_oid = {};
  status = switch_store::object_create(test_key_1, key_1_attrs, key_1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_key_2, key_2_attrs, key_2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_key_2, key_22_attrs, key_22_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_key_3, key_3_attrs, key_3_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  /* create objects using above keys */
  switch_object_id_t oid_1 = {};
  const std::set<attr_w> oid_1_attrs{attr_w(obj3_key1, key_1_oid),
                                     attr_w(obj3_key2, key_2_oid)};
  status = switch_store::object_create(test_object_3, oid_1_attrs, oid_1);
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_object_id_t oid_2 = {};
  const std::set<attr_w> oid_2_attrs{attr_w(obj3_key2, key_2_oid),
                                     attr_w(obj3_key3, key_3_oid)};
  status = switch_store::object_create(test_object_3, oid_2_attrs, oid_2);
  assert(status == SWITCH_STATUS_SUCCESS);

  /* 1 - Duplicate object create with same attrs */
  switch_object_id_t oid_dup1 = {};
  const std::set<attr_w> oid_dup1_attrs{attr_w(obj3_key1, key_1_oid),
                                        attr_w(obj3_key2, key_2_oid)};
  status = switch_store::object_create(test_object_3, oid_dup1_attrs, oid_dup1);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);
  // assert(oid_1 == oid_dup1);

  /* 2 - Duplicate object create with same attrs */
  switch_object_id_t oid_dup2 = {};
  const std::set<attr_w> oid_dup2_attrs{attr_w(obj3_key3, key_3_oid),
                                        attr_w(obj3_key2, key_2_oid)};
  status = switch_store::object_create(test_object_3, oid_dup2_attrs, oid_dup2);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);
  // assert(oid_2 == oid_dup2);

  /* 3 - Duplicate object create with non kg attr*/
  switch_object_id_t oid_dup3 = {};
  const std::set<attr_w> oid_dup3_attrs{attr_w(obj3_key1, key_1_oid),
                                        attr_w(obj3_key2, key_2_oid),
                                        attr_w(obj3_key3, key_3_oid)};
  status = switch_store::object_create(test_object_3, oid_dup3_attrs, oid_dup3);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);
  // assert(oid_1 == oid_dup3);

  /* Create new object with same keys but different value */
  switch_object_id_t oid_22 = {};
  const std::set<attr_w> oid_22_attrs{attr_w(obj3_key2, key_22_oid),
                                      attr_w(obj3_key3, key_3_oid)};
  status = switch_store::object_create(test_object_3, oid_22_attrs, oid_22);
  assert(status == SWITCH_STATUS_SUCCESS);

  /* Create object with mismatched key values */
  switch_object_id_t oid_4 = {};
  const std::set<attr_w> oid_4_attrs{attr_w(obj3_key2, key_1_oid),
                                     attr_w(obj3_key3, key_3_oid)};
  status = switch_store::object_create(test_object_3, oid_4_attrs, oid_4);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::object_delete(oid_1);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid_22);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid_2);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(key_1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(key_22_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(key_2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(key_3_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  uint8_t obj7_1_8 = 8;
  uint64_t obj7_1_64 = 64;
  std::set<attr_w> mand_attrs{attr_w(obj_7_key_1_attr, obj7_1_8)};
  switch_object_id_t oid_7 = {};
  status = smi::api::smi_object_create(test_object_7, mand_attrs, oid_7);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  mand_attrs.insert(attr_w(obj_7_key_2_attr, obj7_1_64));
  status = smi::api::smi_object_create(test_object_7, mand_attrs, oid_7);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid_7);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_key_groups() {
  std::cout << "**** Testing key groups ****" << std::endl;
  switch_status_t status;
  switch_object_id_t zero_oid = {0};

  /* create key attribute objects */
  uint64_t key1 = 1;
  uint64_t key2 = 2;
  uint64_t key3 = 3;
  uint8_t key4 = 1;
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
  status = switch_store::object_create(test_object_3, oid_1_attrs, oid_1);
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_object_id_t oid_2 = {};
  const std::set<attr_w> oid_2_attrs{attr_w(obj3_key2, key_2_oid),
                                     attr_w(obj3_key3, key_3_oid)};
  status = switch_store::object_create(test_object_3, oid_2_attrs, oid_2);
  assert(status == SWITCH_STATUS_SUCCESS);

  /* validate basic get with exact attrs*/
  switch_object_id_t oid_out1 = {0};
  status =
      switch_store::object_id_get_wkey(test_object_3, oid_1_attrs, oid_out1);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(oid_out1.data == oid_1.data);
  switch_object_id_t oid_out2 = {0};
  status =
      switch_store::object_id_get_wkey(test_object_3, oid_2_attrs, oid_out2);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(oid_out2.data == oid_2.data);
  switch_object_id_t oid_out3 = {0};
  const std::set<attr_w> oid_attrs{attr_w(obj3_key3, key_3_oid),
                                   attr_w(obj3_key2, key_2_oid)};
  status = switch_store::object_id_get_wkey(test_object_3, oid_attrs, oid_out3);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(oid_out3.data == oid_2.data);

  /* validate basic get with extra attrs*/
  oid_out1.data = oid_out2.data = 0;
  const std::set<attr_w> oid_11_attrs{attr_w(obj3_key1, key_1_oid),
                                      attr_w(obj3_key2, key_2_oid),
                                      attr_w(obj3_key4, key4)};
  status =
      switch_store::object_id_get_wkey(test_object_3, oid_11_attrs, oid_out1);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(oid_out1.data == oid_1.data);
  const std::set<attr_w> oid_31_attrs{attr_w(obj3_key4, key4),
                                      attr_w(obj3_key2, key_2_oid),
                                      attr_w(obj3_key3, key_3_oid)};
  status =
      switch_store::object_id_get_wkey(test_object_3, oid_31_attrs, oid_out2);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(oid_out2.data == oid_2.data);

  /* Invalid key group set */
  /* Incorrect set */
  switch_object_id_t fail_oid = {0};
  const std::set<attr_w> bad_attrs1{attr_w(obj3_key1, key_1_oid),
                                    attr_w(obj3_key3, key_3_oid)};
  status =
      switch_store::object_id_get_wkey(test_object_3, bad_attrs1, fail_oid);
  assert(fail_oid.data == 0);
  assert(status == SWITCH_STATUS_INVALID_KEY_GROUP);

  /* Single attr in set */
  const std::set<attr_w> bad_attrs2{attr_w(obj3_key1, key_1_oid)};
  status =
      switch_store::object_id_get_wkey(test_object_3, bad_attrs2, fail_oid);
  assert(fail_oid.data == 0);
  assert(status == SWITCH_STATUS_INVALID_KEY_GROUP);

  /* Object not found test */
  const std::set<attr_w> wrong_attrs{attr_w(obj3_key1, key_1_oid),
                                     attr_w(obj3_key2, zero_oid)};
  status =
      switch_store::object_id_get_wkey(test_object_3, wrong_attrs, fail_oid);
  assert(fail_oid.data == 0);

  status = switch_store::object_delete(oid_1);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid_2);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(key_1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(key_2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(key_3_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  switch_object_id_t oid_4 = {}, oid_4_fail = {};
  uint64_t key64 = 1;
  switch_mac_addr_t mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  const std::set<attr_w> object_4_attrs{attr_w(obj_4_key_64_attr, key64),
                                        attr_w(obj_4_key_mac_attr, mac)};
  status = switch_store::object_create(test_object_4, object_4_attrs, oid_4);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(test_auto_6_var == 1);

  // create should fail and var should not change
  status =
      switch_store::object_create(test_object_4, object_4_attrs, oid_4_fail);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);
  assert(test_auto_6_var == 1);

  status = switch_store::object_delete(oid_4);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(test_auto_6_var == 0);
}

void test_object_with_id_hdl() {
  std::cout << "**** Testing objects with id and hdl  ****" << std::endl;
  switch_status_t status;
  switch_object_id_t object_id = {};
  switch_object_id_t empty_object_id = {};
  std::vector<switch_object_id_t> object_handles;

  switch_attr_id_t attr_id =
      test_object_1_info->get_attr_id_from_name("test_attribute_uint64");
  assert(attr_id != 0);

  uint64_t val = 0;
  const std::set<attr_w> attrs{attr_w(attr_id, val)};

  uint32_t id = 3;
  status = switch_store::object_create_by_id(test_object_1, attrs, id);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create_by_id(test_object_invalid, attrs, id);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  object_id = switch_store::object_get_by_id(test_object_1, id);
  assert(object_id.data != SWITCH_NULL_OBJECT_ID);
  object_id = switch_store::object_get_by_id(test_object_invalid, id);
  assert(object_id.data == SWITCH_NULL_OBJECT_ID);

  status = switch_store::object_delete_by_id(test_object_1, id);
  assert(status == SWITCH_STATUS_SUCCESS);

  object_id = switch_store::object_get_by_id(test_object_1, id);
  assert(object_id.data == SWITCH_NULL_OBJECT_ID);

  status = switch_store::object_delete_by_id(test_object_1, id);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND);

  status = switch_store::object_delete_by_id(test_object_invalid, id);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  object_id = switch_store::id_to_handle(test_object_1, id);
  assert(object_id.data != SWITCH_NULL_OBJECT_ID);
  status = switch_store::oid_create(test_object_1, object_id, true);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::oid_create(test_object_invalid, object_id, true);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND);

  status =
      switch_store::oid_get_all_handles(test_object_invalid, object_handles);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND);

  status = switch_store::object_create_by_hdl(test_object_1, attrs, object_id);
  assert(status == SWITCH_STATUS_SUCCESS);
  status =
      switch_store::object_create_by_hdl(test_object_invalid, attrs, object_id);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);
  status =
      switch_store::object_create_by_hdl(test_object_1, attrs, empty_object_id);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::validate_attrs(test_object_1_info, attrs);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::check_for_existing(test_object_1, attrs, object_id);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  status = switch_store::object_delete_by_id(test_object_1, id);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_enums() {
  std::cout << "**** Testing enums ****" << std::endl;
  switch_status_t status;

  const ObjectInfo *test_object_enum_info =
      model_info->get_object_info_from_name("test_object_enum");
  const switch_object_type_t object_type = test_object_enum_info->object_type;
  assert(object_type != 0);
  const switch_attr_id_t attr_id_enum =
      test_object_enum_info->get_attr_id_from_name("test_attribute_enum");
  assert(attr_id_enum != 0);

  // Object Create with valid enum value
  switch_enum_t e{.enumdata = 0}, e_temp = {.enumdata = 0};
  std::set<attr_w> attrs;
  attrs.insert(attr_w(attr_id_enum, e));
  switch_object_id_t oid = {};
  status = switch_store::object_create(object_type, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr_w temp1_attr(attr_id_enum);
  status = switch_store::attribute_get(oid, attr_id_enum, temp1_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  temp1_attr.v_get(e_temp);
  assert(e_temp.enumdata == e.enumdata);

  // Validate enum attributes
  status = switch_store::validate_attrs(test_object_enum_info, attrs);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Object Set with valid enum value
  switch_enum_t valid_enum_value{.enumdata = 2};
  attr_w valid_value(attr_id_enum, valid_enum_value);
  status = switch_store::attribute_set(oid, valid_value);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr_w temp2_attr(attr_id_enum);
  status = switch_store::attribute_get(oid, attr_id_enum, temp2_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  temp2_attr.v_get(e_temp);
  assert(e_temp.enumdata == 2);

  // Object Set with invalid enum value
  switch_enum_t invalid_enum_value{.enumdata = 3};
  attr_w invalid_value(attr_id_enum, invalid_enum_value);
  status = switch_store::attribute_set(oid, invalid_value);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);
  attr_w temp3_attr(attr_id_enum);
  status = switch_store::attribute_get(oid, attr_id_enum, temp3_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  temp3_attr.v_get(e_temp);
  assert(e_temp.enumdata == 2);

  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  oid.data = 0;

  // Object Create with invalid enum value
  std::set<attr_w> invalid_attrs;
  invalid_attrs.insert(attr_w(attr_id_enum, invalid_enum_value));
  oid = {};
  status = switch_store::object_create(object_type, invalid_attrs, oid);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);
}

void test_list() {
  std::cout << "**** Testing lists ****" << std::endl;
  switch_status_t status;

  const ObjectInfo *test_object_list_info =
      model_info->get_object_info_from_name("test_object_list");
  const switch_object_type_t test_object_list =
      test_object_list_info->object_type;
  assert(test_object_list != 0);
  const switch_attr_id_t attr_id_64 =
      test_object_list_info->get_attr_id_from_name(
          "test_attribute_list_uint64");
  assert(attr_id_64 != 0);
  const switch_attr_id_t attr_id_oid =
      test_object_list_info->get_attr_id_from_name("test_attribute_list_oid");
  assert(attr_id_oid != 0);

  switch_object_id_t oid = {};
  attr_w list_uint64(attr_id_64);
  std::vector<uint64_t> u64_list, new_u64_list;
  for (uint64_t i = 0; i < 100; i++) {
    u64_list.push_back(i);
  }
  list_uint64.v_set(u64_list);
  const std::set<attr_w> attrs{list_uint64};

  const switch_attr_id_t attr_id_64signed =
      test_object_list_info->get_attr_id_from_name("test_attribute_list_int64");
  assert(attr_id_64signed != 0);
  const switch_attr_id_t attr_id_oid_s =
      test_object_list_info->get_attr_id_from_name("test_attribute_list_oid");
  assert(attr_id_oid_s != 0);

  switch_object_id_t oid_s = {};
  attr_w list_int64(attr_id_64signed);
  std::vector<uint64_t> s64_list, new_s64_list;
  for (uint64_t i = 0; i < 100; i++) {
    s64_list.push_back(i);
  }
  list_int64.v_set(s64_list);
  const std::set<attr_w> sattrs{list_int64};

  status = switch_store::object_create(test_object_list, sattrs, oid_s);
  assert(status == SWITCH_STATUS_SUCCESS);

  attr_w new_list_int64(attr_id_64signed);
  status = switch_store::attribute_get(oid_s, attr_id_64signed, new_list_int64);
  new_list_int64.v_get(new_s64_list);
  for (auto i = 0; i < 100; i++) {
    assert(s64_list[i] == new_s64_list[i]);
    assert(status == SWITCH_STATUS_SUCCESS);
  }

  status = switch_store::object_delete(oid_s);
  assert(status == SWITCH_STATUS_SUCCESS);
  oid_s.data = 0;

  const std::set<attr_w> tattrs;
  switch_object_id_t oid1 = {}, oid2 = {};
  /* create */
  status = switch_store::object_create(test_object_2, tattrs, oid1);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_object_2, tattrs, oid2);
  assert(status == SWITCH_STATUS_SUCCESS);

  attr_w list_oid(attr_id_oid);
  std::vector<switch_object_id_t> oid_list, new_oid_list;
  oid_list.push_back(oid1);
  oid_list.push_back(oid2);
  list_oid.v_set(oid_list);
  const std::set<attr_w> oid_attrs{list_oid};

  oid = {};
  status = switch_store::object_create(test_object_list, oid_attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_membership() {
  std::cout << "**** Testing membership ****" << std::endl;
  switch_status_t status;
  std::set<attr_w> no_attrs;
  switch_object_id_t oid = {}, moid1 = {}, moid2 = {}, moid3 = {};

  const ObjectInfo *member_ot_info =
      model_info->get_object_info_from_name("test_object_membership");
  const switch_object_type_t member_ot = member_ot_info->object_type;
  assert(member_ot != 0);
  const switch_attr_id_t group_oid =
      member_ot_info->get_attr_id_from_name("test_attribute_oid");
  assert(group_oid != 0);

  status = switch_store::object_create(
      test_object_group_membership_ot, no_attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(test_mem_count == 1);

  const std::set<attr_w> attr1{attr_w(group_oid, oid)};
  const std::set<attr_w> attr2{attr_w(group_oid, oid)};
  const std::set<attr_w> attr3{attr_w(group_oid, oid)};

  std::vector<switch_object_id_t> moids;

  status = switch_store::object_create(member_ot, attr1, moid1);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr_w get_attr1(test_attribute_list_oid);
  status = switch_store::attribute_get(oid, test_attribute_list_oid, get_attr1);
  assert(status == SWITCH_STATUS_SUCCESS);
  moids.clear();
  get_attr1.v_get(moids);
  assert(moids.size() == 1);
  assert(test_mem_count == 2);

  status = switch_store::object_create(member_ot, attr2, moid2);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr_w get_attr2(test_attribute_list_oid);
  status = switch_store::attribute_get(oid, test_attribute_list_oid, get_attr2);
  assert(status == SWITCH_STATUS_SUCCESS);
  moids.clear();
  get_attr2.v_get(moids);
  assert(moids.size() == 2);
  assert(test_mem_count == 3);

  status = switch_store::object_create(member_ot, attr3, moid3);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr_w get_attr3(test_attribute_list_oid);
  status = switch_store::attribute_get(oid, test_attribute_list_oid, get_attr3);
  assert(status == SWITCH_STATUS_SUCCESS);
  moids.clear();
  get_attr3.v_get(moids);
  assert(moids.size() == 3);
  assert(test_mem_count == 4);

  status = switch_store::validate_attrs(member_ot_info, attr3);
  assert(status == SWITCH_STATUS_SUCCESS);

  for (auto temp_oid : moids) {
    std::cout << std::hex << temp_oid.data << std::endl;
  }

  status = switch_store::object_delete(moid1);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr_w get_attr4(test_attribute_list_oid);
  status = switch_store::attribute_get(oid, test_attribute_list_oid, get_attr4);
  assert(status == SWITCH_STATUS_SUCCESS);
  moids.clear();
  get_attr4.v_get(moids);
  assert(moids.size() == 2);
  assert(test_mem_count == 5);

  status = switch_store::object_delete(moid2);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr_w get_attr5(test_attribute_list_oid);
  status = switch_store::attribute_get(oid, test_attribute_list_oid, get_attr5);
  assert(status == SWITCH_STATUS_SUCCESS);
  moids.clear();
  get_attr5.v_get(moids);
  assert(moids.size() == 1);
  assert(test_mem_count == 6);

  status = switch_store::object_delete(moid3);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr_w get_attr6(test_attribute_list_oid);
  status = switch_store::attribute_get(oid, test_attribute_list_oid, get_attr6);
  assert(status == SWITCH_STATUS_SUCCESS);
  moids.clear();
  get_attr6.v_get(moids);
  assert(moids.size() == 0);
  assert(test_mem_count == 7);

  status = switch_store::object_delete(oid);
  assert(test_mem_count == 0);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_ref_tracking() {
  std::cout << "**** Tesing reference tracking ****" << std::endl;
  switch_status_t status;
  switch_object_id_t oid = {};
  std::set<attr_w> no_attrs;
  switch_object_id_t referenced_oid = {};

  const ObjectInfo *test_object_list_info =
      model_info->get_object_info_from_name("test_object_list");
  const switch_object_type_t test_object_list =
      test_object_list_info->object_type;
  assert(test_object_list != 0);
  const switch_attr_id_t attr_id =
      test_object_list_info->get_attr_id_from_name("test_attribute_list_oid");
  assert(attr_id != 0);

  status = switch_store::object_create(test_object_list, no_attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  /* create */
  status = switch_store::object_create(test_object_1, no_attrs, referenced_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::list_v_push(oid, attr_id, referenced_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(referenced_oid);
  assert(status != SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(referenced_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

int create_handler_cnt = 0;
int update_handler_cnt = 0;
int delete_handler_cnt = 0;

switch_status_t test_create_handler(const switch_object_id_t object_id,
                                    const std::set<attr_w> &attrs) {
  (void)attrs;
  create_handler_cnt++;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_attr_id_t attr_id =
      test_object_1_info->get_attr_id_from_name("test_attribute_uint64");
  assert(attr_id != 0);

  status = switch_store::v_set<uint64_t>(object_id, attr_id, 0);
  assert(status == SWITCH_STATUS_SUCCESS);
  return status;
}
switch_status_t test_update_handler(const switch_object_id_t object_id,
                                    const attr_w &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)object_id;
  (void)attrs;
  update_handler_cnt++;
  return status;
}
switch_status_t test_before_delete_handler(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)object_id;

  delete_handler_cnt++;

  return status;
}

switch_status_t test_after_delete_handler(
    const switch_object_type_t object_type, const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)object_type;
  (void)attrs;

  return status;
}

switch_status_t test_before_create_handler(
    const switch_object_type_t object_type, std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)object_type;
  (void)attrs;
  return status;
}

switch_status_t get_counters_handler(const switch_object_id_t object_id,
                                     std::vector<switch_counter_t> &cntrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)object_id;
  (void)cntrs;
  return status;
}

switch_status_t set_counters_handler(const switch_object_id_t object_id,
                                     const std::vector<uint16_t> &cntrs_ids) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)object_id;
  (void)cntrs_ids;
  return status;
}

switch_status_t set_all_counters_handler(const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)object_id;

  return status;
}

bool skip_auto_obj_handler(const switch_object_id_t object_id,
                           const switch_object_type_t auto_ot) {
  bool status = true;
  (void)object_id;
  (void)auto_ot;

  return status;
}

void test_triggers() {
  std::cout << "**** Testing triggers****" << std::endl;
  switch_status_t status;

  /* basic create trigger */

  status =
      switch_store::reg_create_trigs_after(test_object_1, test_create_handler);
  assert(status == SWITCH_STATUS_SUCCESS);

  status =
      switch_store::reg_update_trigs_after(test_object_1, test_update_handler);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::reg_delete_trigs_before(test_object_1,
                                                 test_before_delete_handler);
  assert(status == SWITCH_STATUS_SUCCESS);

  // error scenarios
  status = switch_store::reg_update_trigs_after(test_object_invalid,
                                                test_update_handler);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::reg_create_trigs_after(test_object_invalid,
                                                test_create_handler);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::reg_delete_trigs_before(test_object_invalid,
                                                 test_before_delete_handler);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::reg_delete_trigs_after(test_object_invalid,
                                                test_after_delete_handler);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::reg_update_trigs_before(test_object_invalid,
                                                 test_update_handler);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::reg_create_trigs_before(test_object_invalid,
                                                 test_before_create_handler);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::reg_counter_get_trigs(test_object_invalid,
                                               get_counters_handler);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::reg_counters_set_trigs(test_object_invalid,
                                                set_counters_handler);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::reg_all_counters_set_trigs(test_object_invalid,
                                                    set_all_counters_handler);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::reg_skip_auto_object_trigs(test_object_invalid,
                                                    skip_auto_obj_handler);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  const std::set<attr_w> attrs;
  switch_object_id_t oid = {};
  std::vector<switch_counter_t> cntrs;
  /* create */
  status = switch_store::object_create(test_object_1, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  assert(create_handler_cnt == 1);
  assert(update_handler_cnt == 0);

  status = switch_store::object_counters_get(oid, cntrs);
  assert(status == SWITCH_STATUS_FAILURE);

  status = switch_store::object_counters_clear_all(oid);
  assert(status == SWITCH_STATUS_FAILURE);

  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(delete_handler_cnt == 1);
}

static std::set<int64_t> before_create_trigger_1_resource;
static std::set<int64_t> before_create_trigger_2_resource;
static std::set<int64_t> before_create_trigger_3_resource;
static std::set<int64_t> after_create_trigger_1_resource;
static std::set<int64_t> after_create_trigger_2_resource;
static std::set<int64_t> after_create_trigger_3_resource;

std::queue<switch_object_id_t> test_trigger_oids;
void post_trigger_verify() {
  std::cout << "Post Trigger Verify Resource Utilization" << std::endl;
  auto num_of_oids = test_trigger_oids.size();
  assert(before_create_trigger_1_resource.size() == num_of_oids &&
         before_create_trigger_2_resource.size() == num_of_oids &&
         before_create_trigger_3_resource.size() == num_of_oids &&
         after_create_trigger_1_resource.size() == num_of_oids &&
         after_create_trigger_2_resource.size() == num_of_oids &&
         after_create_trigger_3_resource.size() == num_of_oids);
  return;
}

switch_status_t before_create_trigger_1_handler(
    const switch_object_type_t object_type, std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  auto rand_seed_1_it = attrs.find(
      static_cast<switch_attr_id_t>(test_trig_obj_random_seed_before_1));
  if (rand_seed_1_it != attrs.end()) {
    attrs.erase(rand_seed_1_it);
  }
  const auto before_create_trigger_1_fail_it =
      attrs.find(static_cast<switch_attr_id_t>(before_create_trigger_1_fail));
  if (before_create_trigger_1_fail_it != attrs.end()) {
    bool fail = false;
    status = (before_create_trigger_1_fail_it)->v_get(fail);
    assert(status == SWITCH_STATUS_SUCCESS);
    if (fail) return SWITCH_STATUS_FAILURE;
  }

  // Insert a randomly generated seed as seed 1 attribute
  srand(clock());
  auto it = before_create_trigger_1_resource.insert(rand());
  while (!it.second) {
    it = before_create_trigger_1_resource.insert(rand());
  }
  attr_w random_seed_1_attr(test_trig_obj_random_seed_before_1);
  random_seed_1_attr.v_set(*it.first);
  attrs.insert(random_seed_1_attr);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t before_create_trigger_2_handler(
    const switch_object_type_t object_type, std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  auto rand_seed_2_it = attrs.find(
      static_cast<switch_attr_id_t>(test_trig_obj_random_seed_before_2));
  if (rand_seed_2_it != attrs.end()) {
    attrs.erase(rand_seed_2_it);
  }
  const auto before_create_trigger_2_fail_it =
      attrs.find(static_cast<switch_attr_id_t>(before_create_trigger_2_fail));
  if (before_create_trigger_2_fail_it != attrs.end()) {
    bool fail = false;
    status = (before_create_trigger_2_fail_it)->v_get(fail);
    assert(status == SWITCH_STATUS_SUCCESS);
    if (fail) return SWITCH_STATUS_FAILURE;
  }

  // Insert a randomly generated seed as seed 2 attribute
  srand(clock());
  auto it = before_create_trigger_2_resource.insert(rand());
  while (!it.second) {
    it = before_create_trigger_2_resource.insert(rand());
  }
  attr_w random_seed_2_attr(test_trig_obj_random_seed_before_2);
  random_seed_2_attr.v_set(*it.first);
  attrs.insert(random_seed_2_attr);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t before_create_trigger_3_handler(
    const switch_object_type_t object_type, std::set<attr_w> &attrs) {
  (void)object_type;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  auto rand_seed_3_it = attrs.find(
      static_cast<switch_attr_id_t>(test_trig_obj_random_seed_before_3));
  if (rand_seed_3_it != attrs.end()) {
    attrs.erase(rand_seed_3_it);
  }
  const auto before_create_trigger_3_fail_it =
      attrs.find(static_cast<switch_attr_id_t>(before_create_trigger_3_fail));
  if (before_create_trigger_3_fail_it != attrs.end()) {
    bool fail = false;
    status = (before_create_trigger_3_fail_it)->v_get(fail);
    assert(status == SWITCH_STATUS_SUCCESS);
    if (fail) return SWITCH_STATUS_FAILURE;
  }

  // Insert a randomly generated seed as seed 3 attribute
  srand(clock());
  auto it = before_create_trigger_3_resource.insert(rand());
  while (!it.second) {
    it = before_create_trigger_3_resource.insert(rand());
  }
  attr_w random_seed_3_attr(test_trig_obj_random_seed_before_3);
  random_seed_3_attr.v_set(*it.first);
  attrs.insert(random_seed_3_attr);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t after_delete_trigger_1_handler(
    const switch_object_type_t object_type, const std::set<attr_w> &attrs) {
  (void)object_type;
  (void)attrs;
  auto rand_seed_1_it = attrs.find(
      static_cast<switch_attr_id_t>(test_trig_obj_random_seed_before_1));
  if (rand_seed_1_it == attrs.end()) {
    return SWITCH_STATUS_FAILURE;
  }
  int64_t seed1{};
  rand_seed_1_it->v_get(seed1);
  before_create_trigger_1_resource.erase(seed1);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t after_delete_trigger_2_handler(
    const switch_object_type_t object_type, const std::set<attr_w> &attrs) {
  (void)object_type;
  (void)attrs;
  auto rand_seed_2_it = attrs.find(
      static_cast<switch_attr_id_t>(test_trig_obj_random_seed_before_2));
  if (rand_seed_2_it == attrs.end()) {
    return SWITCH_STATUS_FAILURE;
  }
  int64_t seed2{};
  rand_seed_2_it->v_get(seed2);
  before_create_trigger_2_resource.erase(seed2);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t after_delete_trigger_3_handler(
    const switch_object_type_t object_type, const std::set<attr_w> &attrs) {
  (void)object_type;
  (void)attrs;
  auto rand_seed_3_it = attrs.find(
      static_cast<switch_attr_id_t>(test_trig_obj_random_seed_before_3));
  if (rand_seed_3_it == attrs.end()) {
    return SWITCH_STATUS_FAILURE;
  }
  int64_t seed3{};
  rand_seed_3_it->v_get(seed3);
  before_create_trigger_3_resource.erase(seed3);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t after_create_trigger_1_handler(
    const switch_object_id_t object_id, const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)object_id;
  const auto after_create_trigger_1_fail_it =
      attrs.find(static_cast<switch_attr_id_t>(after_create_trigger_1_fail));
  if (after_create_trigger_1_fail_it != attrs.end()) {
    bool fail = false;
    status = (after_create_trigger_1_fail_it)->v_get(fail);
    assert(status == SWITCH_STATUS_SUCCESS);
    if (fail) return SWITCH_STATUS_FAILURE;
  }

  // Insert a randomly generated seed as seed 3 attribute
  srand(clock());
  auto it = after_create_trigger_1_resource.insert(rand());
  while (!it.second) {
    it = after_create_trigger_1_resource.insert(rand());
  }
  return switch_store::v_set(
      object_id, test_trig_obj_random_seed_after_1, *it.first);
}

switch_status_t after_create_trigger_2_handler(
    const switch_object_id_t object_id, const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)object_id;
  const auto after_create_trigger_2_fail_it =
      attrs.find(static_cast<switch_attr_id_t>(after_create_trigger_2_fail));
  if (after_create_trigger_2_fail_it != attrs.end()) {
    bool fail = false;
    status = (after_create_trigger_2_fail_it)->v_get(fail);
    assert(status == SWITCH_STATUS_SUCCESS);
    if (fail) return SWITCH_STATUS_FAILURE;
  }

  // Insert a randomly generated seed as seed 3 attribute
  srand(clock());
  auto it = after_create_trigger_2_resource.insert(rand());
  while (!it.second) {
    it = after_create_trigger_2_resource.insert(rand());
  }
  return switch_store::v_set(
      object_id, test_trig_obj_random_seed_after_2, *it.first);
}

switch_status_t after_create_trigger_3_handler(
    const switch_object_id_t object_id, const std::set<attr_w> &attrs) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  (void)object_id;
  const auto after_create_trigger_3_fail_it =
      attrs.find(static_cast<switch_attr_id_t>(after_create_trigger_3_fail));
  if (after_create_trigger_3_fail_it != attrs.end()) {
    bool fail = false;
    status = (after_create_trigger_3_fail_it)->v_get(fail);
    assert(status == SWITCH_STATUS_SUCCESS);
    if (fail) return SWITCH_STATUS_FAILURE;
  }

  // Insert a randomly generated seed as seed 3 attribute
  srand(clock());
  auto it = after_create_trigger_3_resource.insert(rand());
  while (!it.second) {
    it = after_create_trigger_3_resource.insert(rand());
  }
  return switch_store::v_set(
      object_id, test_trig_obj_random_seed_after_3, *it.first);
}

switch_status_t before_delete_trigger_1_handler(
    const switch_object_id_t object_id) {
  int64_t seed1{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  status =
      switch_store::v_get(object_id, test_trig_obj_random_seed_after_1, seed1);
  if (status != SWITCH_STATUS_SUCCESS) return status;
  after_create_trigger_1_resource.erase(seed1);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t before_delete_trigger_2_handler(
    const switch_object_id_t object_id) {
  int64_t seed2{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  status =
      switch_store::v_get(object_id, test_trig_obj_random_seed_after_2, seed2);
  if (status != SWITCH_STATUS_SUCCESS) return status;
  after_create_trigger_2_resource.erase(seed2);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t before_delete_trigger_3_handler(
    const switch_object_id_t object_id) {
  int64_t seed3{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  status =
      switch_store::v_get(object_id, test_trig_obj_random_seed_after_3, seed3);
  if (status != SWITCH_STATUS_SUCCESS) return status;
  after_create_trigger_3_resource.erase(seed3);
  return SWITCH_STATUS_SUCCESS;
}

void test_trigger_rollback() {
  std::cout << "**** Testing trigger rollback on object create ****"
            << std::endl;
  switch_status_t status;

  status = switch_store::reg_create_trigs_before(
      test_object_trigger_ot, before_create_trigger_1_handler);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::reg_create_trigs_before(
      test_object_trigger_ot, before_create_trigger_2_handler);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::reg_create_trigs_before(
      test_object_trigger_ot, before_create_trigger_3_handler);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::reg_delete_trigs_before(
      test_object_trigger_ot, before_delete_trigger_1_handler);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::reg_delete_trigs_before(
      test_object_trigger_ot, before_delete_trigger_2_handler);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::reg_delete_trigs_before(
      test_object_trigger_ot, before_delete_trigger_3_handler);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::reg_create_trigs_after(test_object_trigger_ot,
                                                after_create_trigger_1_handler);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::reg_create_trigs_after(test_object_trigger_ot,
                                                after_create_trigger_2_handler);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::reg_create_trigs_after(test_object_trigger_ot,
                                                after_create_trigger_3_handler);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::reg_delete_trigs_after(test_object_trigger_ot,
                                                after_delete_trigger_1_handler);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::reg_delete_trigs_after(test_object_trigger_ot,
                                                after_delete_trigger_2_handler);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::reg_delete_trigs_after(test_object_trigger_ot,
                                                after_delete_trigger_3_handler);
  assert(status == SWITCH_STATUS_SUCCESS);

  struct trigger_attrs {
    bool before_create_trigger_1_fail;
    bool before_create_trigger_2_fail;
    bool before_create_trigger_3_fail;
    bool after_create_trigger_1_fail;
    bool after_create_trigger_2_fail;
    bool after_create_trigger_3_fail;
    bool all_triggers_succeed() {
      return !(before_create_trigger_1_fail || before_create_trigger_2_fail ||
               before_create_trigger_3_fail || after_create_trigger_1_fail ||
               after_create_trigger_2_fail || after_create_trigger_3_fail);
    }
  };
  std::vector<trigger_attrs> objects_attrs = {
      {false, false, false, false, false, false},
      {true, false, false, false, false, false},
      {false, true, false, false, false, false},
      {false, true, true, true, true, false},
      {false, false, false, true, false, false},
      {false, false, false, false, true, false},
      {false, false, false, false, false, true},
  };

  for (auto &&object_attrs : objects_attrs) {
    std::set<attr_w> attrs;
    attrs.emplace(attr_w(before_create_trigger_1_fail,
                         object_attrs.before_create_trigger_1_fail));
    attrs.emplace(attr_w(before_create_trigger_2_fail,
                         object_attrs.before_create_trigger_2_fail));
    attrs.emplace(attr_w(before_create_trigger_3_fail,
                         object_attrs.before_create_trigger_3_fail));
    attrs.emplace(attr_w(after_create_trigger_1_fail,
                         object_attrs.after_create_trigger_1_fail));
    attrs.emplace(attr_w(after_create_trigger_2_fail,
                         object_attrs.after_create_trigger_2_fail));
    attrs.emplace(attr_w(after_create_trigger_3_fail,
                         object_attrs.after_create_trigger_3_fail));

    std::cout << "Creating object with attrs " << attrs << std::endl;
    switch_object_id_t oid{};
    status = switch_store::object_create(test_object_trigger_ot, attrs, oid);
    if (status == SWITCH_STATUS_SUCCESS) {
      test_trigger_oids.push(oid);
    }
    post_trigger_verify();
  }

  while (!test_trigger_oids.empty()) {
    status = switch_store::object_delete(test_trigger_oids.front());
    assert(status == SWITCH_STATUS_SUCCESS);
    test_trigger_oids.pop();
    post_trigger_verify();
  }
  post_trigger_verify();

  return;
}

void test_reevaluate() {
  std::cout << "**** Tesing reevaluate ****" << std::endl;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const std::set<attr_w> attrs;
  int exp_test_object_referred_auto_1_create_update_count = 0,
      exp_test_object_referred_auto_2_create_update_count = 0,
      exp_test_object_referred_auto_1_delete_count = 0,
      exp_test_object_referred_auto_2_delete_count = 0;
  switch_object_id_t referred_oid = {};
  /* create Referred object first*/
  status =
      switch_store::object_create(test_object_referred_ot, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  exp_test_object_referred_auto_1_create_update_count++;
  exp_test_object_referred_auto_2_create_update_count++;
  // Verify that referred objects auto objects are create
  switch_object_id_t referred_auto_1_oid = {};
  switch_object_id_t referred_auto_2_oid = {};
  find_auto_oid(
      referred_oid, test_object_referred_auto_1_ot, referred_auto_1_oid);
  assert(referred_auto_1_oid.data != 0);
  assert(test_object_referred_auto_1_create_update_count ==
         exp_test_object_referred_auto_1_create_update_count);
  find_auto_oid(
      referred_oid, test_object_referred_auto_2_ot, referred_auto_2_oid);
  assert(referred_auto_2_oid.data != 0);
  assert(test_object_referred_auto_2_create_update_count ==
         exp_test_object_referred_auto_2_create_update_count);

  // Verify that referred objects auto objects are updated(re-evaluated), when
  // referred by referrer object
  switch_object_id_t referrer_1_oid = {}, referrer_2_oid = {};
  const std::set<attr_w> referrer_1_attrs;
  status = switch_store::object_create(
      test_object_referrer_1_ot, referrer_1_attrs, referrer_1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr_w referrer_1_attr(
      SWITCH_TEST_OBJECT_REFERRER_1_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID,
      referred_oid);
  status = switch_store::attribute_set(referrer_1_oid, referrer_1_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  exp_test_object_referred_auto_1_create_update_count++;
  exp_test_object_referred_auto_2_create_update_count++;
  exp_test_object_referred_auto_1_delete_count++;
  exp_test_object_referred_auto_2_delete_count++;
  assert(test_object_referred_auto_1_create_update_count ==
         exp_test_object_referred_auto_1_create_update_count);
  assert(test_object_referred_auto_2_create_update_count ==
         exp_test_object_referred_auto_2_create_update_count);
  assert(test_object_referred_auto_1_delete_count ==
         exp_test_object_referred_auto_1_delete_count);
  assert(test_object_referred_auto_2_delete_count ==
         exp_test_object_referred_auto_2_delete_count);

  // Resetting the referrence should trigger a re-evaluation if this is the only
  // reference
  status = switch_store::attribute_set(referrer_1_oid, referrer_1_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  exp_test_object_referred_auto_1_create_update_count++;
  exp_test_object_referred_auto_2_create_update_count++;
  exp_test_object_referred_auto_1_delete_count++;
  exp_test_object_referred_auto_2_delete_count++;
  assert(test_object_referred_auto_1_create_update_count ==
         exp_test_object_referred_auto_1_create_update_count);
  assert(test_object_referred_auto_2_create_update_count ==
         exp_test_object_referred_auto_2_create_update_count);
  assert(test_object_referred_auto_1_delete_count ==
         exp_test_object_referred_auto_1_delete_count);
  assert(test_object_referred_auto_2_delete_count ==
         exp_test_object_referred_auto_2_delete_count);

  // Verify that referred objects auto objects are not updated(re-evaluated), if
  // the object was already referred
  const std::set<attr_w> referrer_2_attrs;
  status = switch_store::object_create(
      test_object_referrer_2_ot, referrer_2_attrs, referrer_2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr_w referrer_2_attr(
      SWITCH_TEST_OBJECT_REFERRER_2_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID,
      referred_oid);
  status = switch_store::attribute_set(referrer_2_oid, referrer_2_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(test_object_referred_auto_1_create_update_count ==
         exp_test_object_referred_auto_1_create_update_count);
  assert(test_object_referred_auto_2_create_update_count ==
         exp_test_object_referred_auto_2_create_update_count);
  assert(test_object_referred_auto_1_delete_count ==
         exp_test_object_referred_auto_1_delete_count);
  assert(test_object_referred_auto_2_delete_count ==
         exp_test_object_referred_auto_2_delete_count);

  // Verify that referred objects auto objects are not updated(re-evaluated),
  // when
  // a reference still exists
  switch_object_id_t referred_null_oid = {};
  attr_w referrer_1_null_attr(
      SWITCH_TEST_OBJECT_REFERRER_1_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID,
      referred_null_oid);
  status = switch_store::attribute_set(referrer_1_oid, referrer_1_null_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(test_object_referred_auto_1_create_update_count ==
         exp_test_object_referred_auto_1_create_update_count);
  assert(test_object_referred_auto_2_create_update_count ==
         exp_test_object_referred_auto_2_create_update_count);
  assert(test_object_referred_auto_1_delete_count ==
         exp_test_object_referred_auto_1_delete_count);
  assert(test_object_referred_auto_2_delete_count ==
         exp_test_object_referred_auto_2_delete_count);

  // Verify that referred objects auto objects are updated(re-evaluated), when
  // last reference goes away
  attr_w referrer_2_null_attr(
      SWITCH_TEST_OBJECT_REFERRER_2_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID,
      referred_null_oid);
  status = switch_store::attribute_set(referrer_2_oid, referrer_2_null_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  exp_test_object_referred_auto_1_delete_count++;
  exp_test_object_referred_auto_2_delete_count++;
  assert(test_object_referred_auto_1_create_update_count ==
         exp_test_object_referred_auto_1_create_update_count);
  assert(test_object_referred_auto_2_create_update_count ==
         exp_test_object_referred_auto_2_create_update_count);
  assert(test_object_referred_auto_1_delete_count ==
         exp_test_object_referred_auto_1_delete_count);
  assert(test_object_referred_auto_2_delete_count ==
         exp_test_object_referred_auto_2_delete_count);

  // Verify that referred objects auto objects are updated(re-evaluated), when
  // referred by referrer object at create time
  switch_object_id_t referrer_1_oid_2 = {};
  std::set<attr_w> referrer_1_attrs_2;
  attr_w referrer_1_attr_2(
      SWITCH_TEST_OBJECT_REFERRER_1_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID,
      referred_oid);
  referrer_1_attrs_2.insert(attr_w(
      SWITCH_TEST_OBJECT_REFERRER_1_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID,
      referred_oid));
  status = switch_store::object_create(
      test_object_referrer_1_ot, referrer_1_attrs_2, referrer_1_oid_2);
  assert(status == SWITCH_STATUS_SUCCESS);
  exp_test_object_referred_auto_1_create_update_count++;
  exp_test_object_referred_auto_2_create_update_count++;
  assert(test_object_referred_auto_1_create_update_count ==
         exp_test_object_referred_auto_1_create_update_count);
  assert(test_object_referred_auto_2_create_update_count ==
         exp_test_object_referred_auto_2_create_update_count);
  assert(test_object_referred_auto_1_delete_count ==
         exp_test_object_referred_auto_1_delete_count);
  assert(test_object_referred_auto_2_delete_count ==
         exp_test_object_referred_auto_2_delete_count);

  // Check no re-evaluation on second reference
  switch_object_id_t referrer_2_oid_2 = {};
  std::set<attr_w> referrer_2_attrs_2;
  attr_w referrer_2_attr_2(
      SWITCH_TEST_OBJECT_REFERRER_2_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID,
      referred_oid);
  referrer_2_attrs_2.insert(attr_w(
      SWITCH_TEST_OBJECT_REFERRER_2_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID,
      referred_oid));
  status = switch_store::object_create(
      test_object_referrer_2_ot, referrer_2_attrs_2, referrer_2_oid_2);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(test_object_referred_auto_1_create_update_count ==
         exp_test_object_referred_auto_1_create_update_count);
  assert(test_object_referred_auto_2_create_update_count ==
         exp_test_object_referred_auto_2_create_update_count);
  assert(test_object_referred_auto_1_delete_count ==
         exp_test_object_referred_auto_1_delete_count);
  assert(test_object_referred_auto_2_delete_count ==
         exp_test_object_referred_auto_2_delete_count);

  // Verify no re-evaluation when a referrer object is deleted
  // but referred still has a referrence from another referrer
  status = switch_store::object_delete(referrer_1_oid_2);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(test_object_referred_auto_1_create_update_count ==
         exp_test_object_referred_auto_1_create_update_count);
  assert(test_object_referred_auto_2_create_update_count ==
         exp_test_object_referred_auto_2_create_update_count);
  assert(test_object_referred_auto_1_delete_count ==
         exp_test_object_referred_auto_1_delete_count);
  assert(test_object_referred_auto_2_delete_count ==
         exp_test_object_referred_auto_2_delete_count);
  // Verify that referred objects auto objects are re-evaluated (deleted) when
  // the last
  // last reference from a referrer object goes away
  status = switch_store::object_delete(referrer_2_oid_2);
  assert(status == SWITCH_STATUS_SUCCESS);
  exp_test_object_referred_auto_1_delete_count++;
  exp_test_object_referred_auto_2_delete_count++;
  assert(test_object_referred_auto_1_create_update_count ==
         exp_test_object_referred_auto_1_create_update_count);
  assert(test_object_referred_auto_2_create_update_count ==
         exp_test_object_referred_auto_2_create_update_count);
  assert(test_object_referred_auto_1_delete_count ==
         exp_test_object_referred_auto_1_delete_count);
  assert(test_object_referred_auto_2_delete_count ==
         exp_test_object_referred_auto_2_delete_count);
  return;
}

void test_multiple_refs_by_single_obj() {
  std::cout << "**** Tesing multiple reference ****" << std::endl;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const std::set<attr_w> attrs;
  switch_object_id_t referred_oid = {};
  switch_object_id_t null_oid = {.data = 0};
  /* create referred object first*/
  status =
      switch_store::object_create(test_object_referred_ot, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  switch_object_id_t referrer_3_oid = {};
  const std::set<attr_w> referrer_3_attrs;
  status = switch_store::object_create(
      test_object_referrer_3_ot, referrer_3_attrs, referrer_3_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::v_set(
      referrer_3_oid,
      SWITCH_TEST_OBJECT_REFERRER_3_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID_1,
      referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::v_set(
      referrer_3_oid,
      SWITCH_TEST_OBJECT_REFERRER_3_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID_2,
      referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::v_set(
      referrer_3_oid,
      SWITCH_TEST_OBJECT_REFERRER_3_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID_3,
      referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(referred_oid);
  assert(status != SWITCH_STATUS_SUCCESS);

  status = switch_store::v_set(
      referrer_3_oid,
      SWITCH_TEST_OBJECT_REFERRER_3_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID_1,
      null_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(referred_oid);
  assert(status != SWITCH_STATUS_SUCCESS);

  status = switch_store::v_set(
      referrer_3_oid,
      SWITCH_TEST_OBJECT_REFERRER_3_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID_2,
      null_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(referred_oid);
  assert(status != SWITCH_STATUS_SUCCESS);

  status = switch_store::v_set(
      referrer_3_oid,
      SWITCH_TEST_OBJECT_REFERRER_3_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID_3,
      null_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  referred_oid = {};
  status =
      switch_store::object_create(test_object_referred_ot, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::v_set(
      referrer_3_oid,
      SWITCH_TEST_OBJECT_REFERRER_3_ATTR_TEST_ATTRIBUTE_REFERRED_OBJECT_ID_1,
      referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(referrer_3_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_attributes() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t oid = {};

  // creating test_object_1
  switch_attr_id_t attr_id =
      test_object_1_info->get_attr_id_from_name("test_attribute_uint64");
  assert(attr_id != 0);
  switch_attr_id_t attr_id_ch =
      test_object_1_info->get_attr_id_from_name("test_attribute_uint8");
  assert(attr_id_ch != 0);
  switch_attr_id_t attr_range =
      test_object_1_info->get_attr_id_from_name("test_attribute_range");
  assert(attr_range != 0);
  switch_attr_id_t attr_str =
      test_object_1_info->get_attr_id_from_name("test_attribute_string");
  assert(attr_str != 0);

  uint64_t val = 0;
  uint8_t ch;
  switch_range_t val_range = {};
  switch_string_t text = {};

  const std::set<attr_w> attrs{attr_w(attr_id, val),
                               attr_w(attr_id_ch, ch),
                               attr_w(attr_range, val_range),
                               attr_w(attr_str, text)};

  status = switch_store::object_create(test_object_1, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // object exists
  status = switch_store::object_exists(oid);
  assert(status == 1);

  // createing test_object_list
  const ObjectInfo *test_object_list_info =
      model_info->get_object_info_from_name("test_object_list");
  const switch_object_type_t test_object_list =
      test_object_list_info->object_type;
  assert(test_object_list != 0);
  const switch_attr_id_t attr_id_list_ch =
      test_object_list_info->get_attr_id_from_name("test_attribute_list_uint8");
  assert(attr_id_list_ch != 0);
  const switch_attr_id_t attr_id_list_uint64 =
      test_object_list_info->get_attr_id_from_name(
          "test_attribute_list_uint64");
  assert(attr_id_list_uint64 != 0);
  const switch_attr_id_t attr_id_list_int64 =
      test_object_list_info->get_attr_id_from_name("test_attribute_list_int64");
  assert(attr_id_list_int64 != 0);
  const switch_attr_id_t attr_id_list_int32 =
      test_object_list_info->get_attr_id_from_name(
          "test_attribute_list_uint32");
  assert(attr_id_list_int32 != 0);
  const switch_attr_id_t attr_id_oids =
      test_object_list_info->get_attr_id_from_name("test_attribute_list_oid");
  assert(attr_id_oids != 0);
  const switch_attr_id_t attr_id_prefix =
      test_object_list_info->get_attr_id_from_name(
          "test_attribute_list_prefix");
  assert(attr_id_prefix != 0);
  const switch_attr_id_t attr_id_addr =
      test_object_list_info->get_attr_id_from_name("test_attribute_list_ip");
  assert(attr_id_addr != 0);

  std::vector<uint8_t> ch1;
  std::vector<uint64_t> vals;
  std::vector<int64_t> val_list;
  std::vector<uint32_t> list_int;
  std::vector<switch_object_id_t> oids;
  std::vector<switch_ip_prefix_t> new_prefixes;
  std::vector<switch_ip_address_t> addrs;

  const std::set<attr_w> object_list_attrs{attr_w(attr_id_list_ch, ch1),
                                           attr_w(attr_id_list_uint64, vals),
                                           attr_w(attr_id_list_int64, val_list),
                                           attr_w(attr_id_list_int32, list_int),
                                           attr_w(attr_id_oids, oids),
                                           attr_w(attr_id_prefix, new_prefixes),
                                           attr_w(attr_id_addr, addrs)};
  switch_object_id_t oid_list = {};
  status = switch_store::object_create(
      test_object_list, object_list_attrs, oid_list);
  assert(status == SWITCH_STATUS_SUCCESS);

  // list_v_get unsigned long using test_object_1

  uint64_t val_out = 0;
  status = switch_store::list_v_get(oid, attr_id, 0, val_out);
  assert(status != SWITCH_STATUS_SUCCESS);

  // v_set attribute unsigned char
  status = switch_store::v_set(oid_list, attr_id_list_ch, ch1);
  assert(status == SWITCH_STATUS_SUCCESS);

  // v_get attribute unsigned char
  status = switch_store::v_get(oid, attr_id_ch, ch);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::v_get(oid, attr_id_ch, ch1);
  assert(status == SWITCH_STATUS_SUCCESS);

  // v_set attribute unsigned long
  status = switch_store::v_set(oid_list, attr_id_list_uint64, vals);
  assert(status == SWITCH_STATUS_SUCCESS);

  // v_get attribute unsigned long
  status = switch_store::v_get(oid, attr_id, val);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::v_get(oid, attr_id, vals);
  assert(status == SWITCH_STATUS_SUCCESS);

  // v_set attribute long
  status = switch_store::v_set(oid_list, attr_id_list_int64, val_list);
  assert(status == SWITCH_STATUS_SUCCESS);

  // v_set attribute unsigned int
  status = switch_store::v_set(oid_list, attr_id_list_int32, list_int);
  assert(status == SWITCH_STATUS_SUCCESS);

  // v_set attribute object ids

  status = switch_store::v_set(oid_list, attr_id_oids, oids);
  assert(status == SWITCH_STATUS_SUCCESS);

  // v_set attribute range
  std::vector<switch_range_t> val_ranges;
  status = switch_store::v_set(oid, attr_range, val_range);
  assert(status == SWITCH_STATUS_SUCCESS);

  // range vector is not supported
  status = switch_store::v_set(oid, attr_range, val_ranges);
  assert(status != SWITCH_STATUS_SUCCESS);

  // v_get attribute range
  status = switch_store::v_get(oid, attr_range, val_range);
  assert(status == SWITCH_STATUS_SUCCESS);

  // range vector get is not supported
  status = switch_store::v_get(oid, attr_range, val_ranges);
  assert(status != SWITCH_STATUS_SUCCESS);

  // v_get attribute string
  std::vector<switch_string_t> texts;
  status = switch_store::v_get(oid, attr_str, text);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::v_get(oid, attr_str, texts);
  assert(status == SWITCH_STATUS_SUCCESS);

  // v_get attribute mac
  switch_mac_addr_t mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  switch_object_id_t oid_4 = {};
  const std::set<attr_w> object_4_attrs{attr_w(obj_4_key_mac_attr, mac)};
  status = switch_store::object_create(test_object_4, object_4_attrs, oid_4);
  assert(status == SWITCH_STATUS_SUCCESS);

  switch_attr_id_t attr_mac =
      test_object_4_info->get_attr_id_from_name("test_mac");
  assert(attr_mac != 0);

  switch_mac_addr_t new_mac = {};
  std::vector<switch_mac_addr_t> new_macs;
  status = switch_store::v_get(oid_4, attr_mac, new_mac);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::v_get(oid_4, attr_mac, new_macs);
  assert(status == SWITCH_STATUS_SUCCESS);

  // v_get attribute prefix
  switch_object_id_t oid_3 = {};
  switch_ip_prefix_t prefix;
  const std::set<attr_w> object_3_attrs{attr_w(obj_3_key_prefix_attr, prefix)};
  status = switch_store::object_create(test_object_3, object_3_attrs, oid_3);
  assert(status == SWITCH_STATUS_SUCCESS);

  switch_attr_id_t attr_prefix =
      test_object_3_info->get_attr_id_from_name("test_attribute_ip_prefix");
  assert(attr_prefix != 0);

  switch_ip_prefix_t new_prefix;
  status = switch_store::v_get(oid_3, attr_prefix, new_prefix);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::v_get(oid_3, attr_prefix, new_prefixes);
  assert(status == SWITCH_STATUS_SUCCESS);

  // set attribute ip_prefix
  status = switch_store::v_set(oid_3, attr_prefix, new_prefix);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::v_set(oid_list, attr_id_prefix, new_prefixes);
  assert(status == SWITCH_STATUS_SUCCESS);

  // set attribute ip_addr

  status = switch_store::v_set(oid_list, attr_id_addr, addrs);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_restore_stats_cache(1);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_info_dump(NULL);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);
}

void test_lock(void) {
  /*
   * this test verifies recursive object lock
   */
  using std::chrono::seconds;
  int rc;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const std::set<attr_w> attrs;
  switch_object_id_t oid{};
  std::mutex m;
  std::condition_variable cv;
  int thread2_rc;
  bool thread2_locked = 0;

  status = switch_store::object_create(test_object_1, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  rc = switch_store::object_try_lock(oid);
  assert(rc != 0);

  std::thread thread2([&] {
    thread2_rc = switch_store::object_try_lock(oid);
    {
      std::lock_guard<std::mutex> lk{m};
      thread2_locked = 1;
    }
    cv.notify_all();

    if (thread2_rc) switch_store::object_unlock(oid);
  });

  /* lock the object second time. now the object needs to be unlocked twice to
   * be available to thread2 */
  rc = switch_store::object_try_lock(oid);
  assert(rc != 0);

  /* unlock first time and verify thread2 is not able to lock the object */
  switch_store::object_unlock(oid);
  {
    std::unique_lock<std::mutex> lk(m);
    if (cv.wait_for(lk, seconds(2), [&] { return thread2_locked; })) {
      assert(!"An object is locked by two threads simultaneously");
    }
  }

  /* unlock second time and verify thread2 is able to lock the object */
  switch_store::object_unlock(oid);
  {
    std::unique_lock<std::mutex> lk{m};
    if (cv.wait_for(lk, seconds(2), [&] { return thread2_locked; })) {
      assert(thread2_rc != 0);
    } else {
      assert(!"Failed to lock a released object");
    }
  }

  switch_store::object_delete(oid);
  thread2.join();
}

int main(int argc, char **argv) {
  (void)argc;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  status = switch_store::object_info_init(NULL, false, NULL);
  assert(status == SWITCH_STATUS_FAILURE);

  const char *const test_model_name = TESTDATADIR "/test/test_model.json";
  status = switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);

  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  init_objects();

  REGISTER_OBJECT(test_object_referred_auto_1, test_object_referred_auto_1_ot);
  REGISTER_OBJECT(test_object_referred_auto_2, test_object_referred_auto_2_ot);
  REGISTER_OBJECT(test_auto_6, SWITCH_OBJECT_TYPE_TEST_AUTO_6);
  REGISTER_OBJECT(test_mem_auto_obj_1,
                  SWITCH_OBJECT_TYPE_TEST_MEMBERSHIP_AUTO_OBJECT);

  if (argv[1] && !strcmp(argv[1], "test_lock")) {
    test_lock();
    exit(0);
  }

  test_object_basic();
  test_object_key_groups();
  test_object_with_id_hdl();
  test_list();
  test_enums();
  test_membership();
  test_ref_tracking();
  test_triggers();
  test_trigger_rollback();
  test_reevaluate();
  test_multiple_refs_by_single_obj();
  test_attributes();

  printf("\n\nAll tests passed!\n");
  return 0;
}
