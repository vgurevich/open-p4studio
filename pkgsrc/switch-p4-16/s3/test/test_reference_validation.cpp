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

#include "gen-model/reference_validation.h"
#include "bf_switch/bf_switch_types.h"
#include "s3/attribute.h"
#include "s3/attribute_util.h"
#include "s3/switch_store.h"
#include "s3/factory.h"

using namespace smi;

const char test_model_name[] = TESTDATADIR "/test/test_reference.json";
static ModelInfo *model_info = NULL;
static switch_object_type_t user_referred_object_type;
static switch_object_type_t user_referrer_object_type;
static switch_object_type_t auto_referrer_object_type;
static switch_object_type_t auto_referred_object_type;
static switch_object_type_t user_referrer_list_object_type;
static switch_object_type_t auto_referrer_list_object_type;
static switch_object_type_t user_internal_reference_object_type;
static switch_object_type_t user_read_only_reference_object_type;
static switch_object_type_t user_internal_read_only_reference_object_type;
static switch_object_type_t user_referred_membership_object_type;
static switch_object_type_t user_membership_object_type;
static switch_object_type_t user_referrer_membership_object_type;

static const ObjectInfo *user_referred_object_info;
static const ObjectInfo *user_referrer_object_info;
static const ObjectInfo *auto_referrer_object_info;
static const ObjectInfo *auto_referred_object_info;
static const ObjectInfo *user_referrer_list_object_info;
static const ObjectInfo *auto_referrer_list_object_info;
static const ObjectInfo *user_internal_reference_object_info;
static const ObjectInfo *user_read_only_reference_object_info;
static const ObjectInfo *user_internal_read_only_reference_object_info;
static const ObjectInfo *user_referred_membership_object_info;
static const ObjectInfo *user_membership_object_info;
static const ObjectInfo *user_referrer_membership_object_info;

static switch_attr_id_t u_referred_attr_u64;
static switch_attr_id_t u_referrer_attr_referred_oid;
static switch_attr_id_t a_referrer_attr_referred_oid;
static switch_attr_id_t a_referred_attr_u64;
static switch_attr_id_t u_referrer_attr_referred_oids_list;
static switch_attr_id_t a_referrer_attr_referred_oids_list;
static switch_attr_id_t u_referrer_attr_internal_referred_oid;
static switch_attr_id_t u_referrer_attr_ro_referred_oid;
static switch_attr_id_t u_referrer_attr_internal_ro_referred_oid;
static switch_attr_id_t u_referred_membership_attr_member_of_oid;
static switch_attr_id_t u_membership_attr_members;
static switch_attr_id_t u_referrer_membership_attr_referred_oid;

class auto_referrer_object : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_AUTO_REFERRER_OBJECT;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_AUTO_REFERRER_OBJECT_ATTR_A_REFERRER_ATTR_REFERRED_OID;

 public:
  auto_referrer_object(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }
};

void init_objects() {
  user_referred_object_info =
      model_info->get_object_info_from_name("user_referred_object");
  user_referrer_object_info =
      model_info->get_object_info_from_name("user_referrer_object");
  auto_referrer_object_info =
      model_info->get_object_info_from_name("auto_referrer_object");
  auto_referred_object_info =
      model_info->get_object_info_from_name("auto_referred_object");
  user_referrer_list_object_info =
      model_info->get_object_info_from_name("user_referrer_list_object");
  auto_referrer_list_object_info =
      model_info->get_object_info_from_name("auto_referrer_list_object");
  user_internal_reference_object_info =
      model_info->get_object_info_from_name("user_internal_reference_object");
  user_read_only_reference_object_info =
      model_info->get_object_info_from_name("user_read_only_reference_object");
  user_internal_read_only_reference_object_info =
      model_info->get_object_info_from_name(
          "user_internal_read_only_reference_object");
  user_referred_membership_object_info =
      model_info->get_object_info_from_name("user_referred_membership_object");
  user_membership_object_info =
      model_info->get_object_info_from_name("user_membership_object");
  user_referrer_membership_object_info =
      model_info->get_object_info_from_name("user_referrer_membership_object");

  user_referred_object_type = user_referred_object_info->object_type;
  assert(user_referred_object_type != 0);
  user_referrer_object_type = user_referrer_object_info->object_type;
  assert(user_referrer_object_type != 0);
  auto_referrer_object_type = auto_referrer_object_info->object_type;
  assert(auto_referrer_object_type != 0);
  auto_referred_object_type = auto_referred_object_info->object_type;
  assert(auto_referred_object_type != 0);
  user_referrer_list_object_type = user_referrer_list_object_info->object_type;
  assert(user_referrer_list_object_type != 0);
  auto_referrer_list_object_type = auto_referrer_list_object_info->object_type;
  assert(auto_referrer_list_object_type != 0);
  user_internal_reference_object_type =
      user_internal_reference_object_info->object_type;
  assert(user_internal_reference_object_type != 0);
  user_read_only_reference_object_type =
      user_read_only_reference_object_info->object_type;
  assert(user_read_only_reference_object_type != 0);
  user_internal_read_only_reference_object_type =
      user_internal_read_only_reference_object_info->object_type;
  assert(user_internal_read_only_reference_object_type != 0);
  user_referred_membership_object_type =
      user_referred_membership_object_info->object_type;
  assert(user_referred_membership_object_type != 0);
  user_membership_object_type = user_membership_object_info->object_type;
  assert(user_membership_object_type != 0);
  user_referrer_membership_object_type =
      user_referrer_membership_object_info->object_type;
  assert(user_referrer_membership_object_type != 0);

  u_referred_attr_u64 =
      user_referred_object_info->get_attr_id_from_name("u_referred_attr_u64");
  assert(u_referred_attr_u64 != 0);
  u_referrer_attr_referred_oid =
      user_referrer_object_info->get_attr_id_from_name(
          "u_referrer_attr_referred_oid");
  assert(u_referrer_attr_referred_oid != 0);
  a_referrer_attr_referred_oid =
      auto_referrer_object_info->get_attr_id_from_name(
          "a_referrer_attr_referred_oid");
  assert(a_referrer_attr_referred_oid != 0);
  a_referred_attr_u64 =
      auto_referred_object_info->get_attr_id_from_name("a_referred_attr_u64");
  assert(a_referred_attr_u64 != 0);
  u_referrer_attr_referred_oids_list =
      user_referrer_list_object_info->get_attr_id_from_name(
          "u_referrer_attr_referred_oids_list");
  assert(u_referrer_attr_referred_oids_list != 0);
  a_referrer_attr_referred_oids_list =
      auto_referrer_list_object_info->get_attr_id_from_name(
          "a_referrer_attr_referred_oids_list");
  assert(a_referrer_attr_referred_oids_list != 0);
  u_referrer_attr_internal_referred_oid =
      user_internal_reference_object_info->get_attr_id_from_name(
          "u_referrer_attr_internal_referred_oid");
  assert(u_referrer_attr_internal_referred_oid != 0);
  u_referrer_attr_ro_referred_oid =
      user_read_only_reference_object_info->get_attr_id_from_name(
          "u_referrer_attr_ro_referred_oid");
  assert(u_referrer_attr_ro_referred_oid != 0);
  u_referrer_attr_internal_ro_referred_oid =
      user_internal_read_only_reference_object_info->get_attr_id_from_name(
          "u_referrer_attr_internal_ro_referred_oid");
  assert(u_referrer_attr_internal_ro_referred_oid != 0);
  u_referred_membership_attr_member_of_oid =
      user_referred_membership_object_info->get_attr_id_from_name(
          "u_referred_membership_attr_member_of_oid");
  assert(u_referred_membership_attr_member_of_oid != 0);
  u_membership_attr_members =
      user_membership_object_info->get_attr_id_from_name(
          "u_membership_attr_members");
  assert(u_membership_attr_members != 0);
  u_referrer_membership_attr_referred_oid =
      user_referrer_membership_object_info->get_attr_id_from_name(
          "u_referrer_membership_attr_referred_oid");
  assert(u_referrer_membership_attr_referred_oid != 0);

  REGISTER_OBJECT(auto_referrer_object,
                  SWITCH_OBJECT_TYPE_AUTO_REFERRER_OBJECT);
}

void test_user_user_reference() {
  std::cout << "**** Testing object delete when one user object references "
               "another user object ****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{}, oid_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Create Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  attrs.clear();

  // Create referrer object that refers above referred object
  std::cout << "Create Referrer Object that refers above referred object"
            << std::endl;
  attr_w oid_attr(u_referrer_attr_referred_oid);
  oid_attr.v_set(referred_oid);
  attrs.insert(oid_attr);
  status = switch_store::object_create(
      user_referrer_object_type, attrs, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");
  attrs.clear();

  // Test Case 1: Delete Referred Object. Should Return Object In Use Error.
  // Also neither the referred nor the referrer
  // object should be deleted from the database
  std::cout << "# Test Case 1: Delete Referred Object, which is still referred "
               "by Referrer object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_RESOURCE_IN_USE &&
         "Object Delete for Referred Object should fail with resource in use "
         "error (but it did not), since it is still in use be referrer "
         "object!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_SUCCESS &&
         u_referred_attr_u64_val_out == u_referred_attr_u64_val_in &&
         "Attribute get failed for referred object!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_referred_oid, oid_val_out);
  assert(status == SWITCH_STATUS_SUCCESS && oid_val_out == referred_oid &&
         "Referrer object attribute referred oid get failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Referrer Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referrer object delete failed!");

  // Test Case 3: Now Delete the Referred Object. Should Succeed
  std::cout << "# Test Case 3: Delete Referred Object" << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referred object delete failed!");
  std::cout << std::endl;
}

void test_user_user_list_reference() {
  std::cout << "**** Testing object delete when one user object references "
               "another user object via OID list****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{};
  std::vector<switch_object_id_t> oids_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Create Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  attrs.clear();

  // Create referrer object that refers above referred object
  std::cout << "Create Referrer Object that refers above referred object"
            << std::endl;
  attr_w oids_attr(u_referrer_attr_referred_oids_list);
  std::vector<switch_object_id_t> oids = {referred_oid};
  oids_attr.v_set(oids);
  attrs.insert(oids_attr);
  status = switch_store::object_create(
      user_referrer_list_object_type, attrs, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");
  attrs.clear();

  // Test Case 1: Delete Referred Object. Should Return Object In Use Error.
  // Also neither the referred nor the referrer
  // object should be deleted from the database
  std::cout << "# Test Case 1: Delete Referred Object, which is still referred "
               "by Referrer object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_RESOURCE_IN_USE &&
         "Object Delete for Referred Object should fail with resource in use "
         "error (but it did not), since it is still in use be referrer "
         "object!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_SUCCESS &&
         u_referred_attr_u64_val_out == u_referred_attr_u64_val_in &&
         "Attribute get failed for referred object!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_referred_oids_list, oids_val_out);
  assert(status == SWITCH_STATUS_SUCCESS &&
         oids_val_out.front() == referred_oid &&
         "Referrer object attribute referred oid get failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Referrer Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referrer object delete failed!");

  // Test Case 3: Now Delete the Referred Object. Should Succeed
  std::cout << "# Test Case 3: Delete Referred Object" << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referred object delete failed!");
  std::cout << std::endl;
}

void test_auto_user_reference() {
  std::cout << "**** Testing object delete when one auto object references "
               "another user object ****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{}, oid_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Create Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");

  // Auto object that is child of this user object will automatically be created
  // by the factory infra
  status = find_auto_oid(referred_oid, auto_referrer_object_type, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");

  // Test Case 1: Delete Referred User Object. Both Referred User and Referrer
  // Auto object should be deleted.
  std::cout
      << "# Test Case 1: Delete Referred User Object, which is still referred "
         "by Referrer Auto object"
      << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Object Delete for Referred User Object referenced by Auto Referrer "
         "Object failed!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND &&
         "Attribute get for Deleted Referred Use Object should return Item Not "
         "Found Error!");
  status = switch_store::v_get(
      referrer_oid, a_referrer_attr_referred_oid, oid_val_out);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND &&
         "Auto Referrer Object attribute get should fail with error Item Not "
         "Found after (Parent) Referred User Object is deleted!");

  // Re Create referred object
  referred_oid = {};
  std::cout << "Recreating Referred Object" << std::endl;
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");

  // Auto object that is child of this user object will automatically be created
  // by the factory infra
  status = find_auto_oid(referred_oid, auto_referrer_object_type, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Referrer Auto Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Referrer (Auto) object delete failed!");

  // Test Case 3: Now Delete the Referred Object. Should Succeed
  std::cout << "# Test Case 3: Delete Referred Object" << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Referred (User) object delete failed!");
  std::cout << std::endl;
}

void test_internal_user_user_reference() {
  std::cout << "**** Testing object delete when an internal User Object "
               "(Internal) references "
               "another user object ****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{}, oid_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Creating Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  attrs.clear();

  // Create an Internal Object (INTERNAL) that refers above Referred object
  std::cout << "Creating an Internal Object that refers above referred object"
            << std::endl;
  attr_w oid_attr(u_referrer_attr_referred_oid);
  oid_attr.v_set(referred_oid);
  attrs.insert(oid_attr);
  status = switch_store::object_create(
      user_referrer_object_type, attrs, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");
  attrs.clear();
  status = switch_store::v_set(
      referrer_oid, SWITCH_USER_REFERRER_OBJECT_ATTR_INTERNAL_OBJECT, true);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Failed to mark User Referrer Object as Internal/Internal!");

  // Test Case 1: Delete Referred Object. Should Succeed. References established
  // via Internally Created Objects are
  // assumed to be handled correctly by User Code via pre delete triggers.
  std::cout << "# Test Case 1: Delete Referred Object, which is still referred "
               "by Internal Referrer object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Object Delete for Referred Object should succeed even though it is "
         "being referenced by Referrer Object, since the Referrer Object is "
         "marked as Internal/Internal!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND &&
         "Attribute get for Deleted Referred User Object should return Item "
         "Not Found Error!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_referred_oid, oid_val_out);
  assert(status == SWITCH_STATUS_SUCCESS && oid_val_out == referred_oid &&
         "Referrer object attribute referred oid get failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Internal Referrer Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Internal/Internal Referrer object delete failed!");
  std::cout << std::endl;
}

void test_internal_user_user_list_reference() {
  std::cout << "**** Testing object delete when an internal User Object "
               "(Internal) references "
               "another user object via OID list ****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{};
  std::vector<switch_object_id_t> oids_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Creating Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  attrs.clear();

  // Create an Internal Object (INTERNAL) that refers above Referred object
  std::cout << "Creating an Internal Object that refers above referred object"
            << std::endl;
  attr_w oids_attr(u_referrer_attr_referred_oids_list);
  std::vector<switch_object_id_t> oids = {referred_oid};
  oids_attr.v_set(oids);
  attrs.insert(oids_attr);
  status = switch_store::object_create(
      user_referrer_list_object_type, attrs, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");
  attrs.clear();
  status =
      switch_store::v_set(referrer_oid,
                          SWITCH_USER_REFERRER_LIST_OBJECT_ATTR_INTERNAL_OBJECT,
                          true);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Failed to mark User Referrer Object as Internal/Internal!");

  // Test Case 1: Delete Referred Object. Should Succeed. References established
  // via Internally Created Objects are
  // assumed to be handled correctly by User Code via pre delete triggers.
  std::cout << "# Test Case 1: Delete Referred Object, which is still referred "
               "by Internal Referrer object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Object Delete for Referred Object should succeed even though it is "
         "being referenced by Referrer Object, since the Referrer Object is "
         "marked as Internal/Internal!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND &&
         "Attribute get for Deleted Referred User Object should return Item "
         "Not Found Error!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_referred_oids_list, oids_val_out);
  assert(status == SWITCH_STATUS_SUCCESS &&
         oids_val_out.front() == referred_oid &&
         "Referrer object attribute referred oid get failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Internal Referrer Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Internal/Internal Referrer object delete failed!");
  std::cout << std::endl;
}

switch_status_t after_user_referred_object_create(
    const switch_object_id_t object_id, const std::set<attr_w> &attrs) {
  (void)attrs;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_object_id_t referrer_oid{};
  std::set<attr_w> referrer_attrs;
  attr_w oid_attr(u_referrer_attr_referred_oid);
  oid_attr.v_set(object_id);
  referrer_attrs.insert(oid_attr);
  status = switch_store::object_create(
      user_referrer_object_type, referrer_attrs, referrer_oid);
  if (status == SWITCH_STATUS_SUCCESS) {
    status = switch_store::v_set(
        referrer_oid, SWITCH_USER_REFERRER_OBJECT_ATTR_INTERNAL_OBJECT, true);
  }
  return status;
}

switch_status_t before_user_referred_object_delete(
    const switch_object_id_t object_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  auto &refs =
      switch_store::get_object_references(object_id, user_referrer_object_type);
  if (refs.size() == 0) return SWITCH_STATUS_SUCCESS;
  for (const auto &ref : refs) {
    status |= switch_store::object_delete(ref.oid);
  }
  return status;
}

// Below test case demonstrated how internal objects are created and managed via
// triggers
void test_internal_user_user_reference_with_triggers() {
  std::cout << "**** Testing object delete when an internal User Object "
               "(Internal) references "
               "another user object (Internal Object managed via triggers)****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{}, oid_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_store::reg_create_trigs_after(user_referred_object_type,
                                       &after_user_referred_object_create);
  switch_store::reg_delete_trigs_before(user_referred_object_type,
                                        &before_user_referred_object_delete);

  // Create referred object
  std::cout << "Creating Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  attrs.clear();

  // Get Internal Object (INTERNAL) that refers above Referred object
  std::cout << "Fetching Internal Object that refers above Referred object"
            << std::endl;
  const auto &refs = switch_store::get_object_references(
      referred_oid, user_referrer_object_type);
  assert(refs.size() == 1 &&
         "There should be exactly 1 Reference (From User Referred Object to "
         "Internal User Referrer Object!");
  referrer_oid = refs.begin()->oid;
  assert(referrer_oid.data != 0);
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_referred_oid, oid_val_out);
  assert(status == SWITCH_STATUS_SUCCESS && oid_val_out == referred_oid &&
         "Referrer attr referred oid is incorrect!");

  // Test Case 1: Delete Referred Object. Should Succeed. References established
  // via Internally Created Objects are
  // assumed to be handled correctly by User Code via pre delete triggers.
  std::cout << "# Test Case 1: Delete Referred Object, which is still referred "
               "by Internal Referrer object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Object Delete for Referred Object Failed! Should succeed even though "
         "it is being referenced by Referrer Object, since the Referrer Object "
         "is marked as Internal/Internal!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND &&
         "Attribute get for Deleted Referred User Object should return Item "
         "Not Found Error!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_referred_oid, oid_val_out);
  assert(
      status == SWITCH_STATUS_ITEM_NOT_FOUND &&
      "Internal/Internal Referrer object should have deleted during referred "
      "object delete!");
  std::cout << std::endl;
}

void test_user_internal_user_reference() {
  std::cout << "**** Testing object delete when a User Object "
               "references "
               "a Internal User Object ****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{}, oid_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Creating Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  // Mark this object as Internal/Internal
  status = switch_store::v_set(
      referred_oid, SWITCH_USER_REFERRED_OBJECT_ATTR_INTERNAL_OBJECT, true);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Failed to mark User Referred Object as Internal/Internal!");
  attrs.clear();

  // Create an User Object that refers above Referred object
  std::cout << "Creating a User Object that refers above Internal Object"
            << std::endl;
  attr_w oid_attr(u_referrer_attr_referred_oid);
  oid_attr.v_set(referred_oid);
  attrs.insert(oid_attr);
  status = switch_store::object_create(
      user_referrer_object_type, attrs, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");
  attrs.clear();

  // Test Case 1: Delete Referred Object. Should Return Object In Use Error.
  // Also neither the referred nor the referrer
  // object should be deleted from the database
  std::cout << "# Test Case 1: Delete Internal/Internal Referred Object, which "
               "is still referred "
               "by Referrer object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_RESOURCE_IN_USE &&
         "Object Delete for Internal/Internal Referred Object should fail with "
         "resource in use "
         "error (but it did not), since it is still in use by referrer "
         "object!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_SUCCESS &&
         u_referred_attr_u64_val_out == u_referred_attr_u64_val_in &&
         "Attribute get failed for referred object!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_referred_oid, oid_val_out);
  assert(status == SWITCH_STATUS_SUCCESS && oid_val_out == referred_oid &&
         "Referrer object attribute referred oid get failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Referrer Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referrer object delete failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 3: Delete Referred Object" << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Internal/Internal Referred Object delete failed!");
  std::cout << std::endl;
}

void test_user_internal_user_list_reference() {
  std::cout << "**** Testing object delete when a User Object "
               "references "
               "a Internal User Object OID List****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{};
  std::vector<switch_object_id_t> oids_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Creating Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  // Mark this object as Internal/Internal
  status = switch_store::v_set(
      referred_oid, SWITCH_USER_REFERRED_OBJECT_ATTR_INTERNAL_OBJECT, true);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Failed to mark User Referred Object as Internal/Internal!");
  attrs.clear();

  // Create an User Object that refers above Referred object
  std::cout << "Creating a User Object that refers above Internal Object"
            << std::endl;
  attr_w oids_attr(u_referrer_attr_referred_oids_list);
  std::vector<switch_object_id_t> oids = {referred_oid};
  oids_attr.v_set(oids);
  attrs.insert(oids_attr);
  status = switch_store::object_create(
      user_referrer_list_object_type, attrs, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");
  attrs.clear();

  // Test Case 1: Delete Referred Object. Should Return Object In Use Error.
  // Also neither the referred nor the referrer
  // object should be deleted from the database
  std::cout << "# Test Case 1: Delete Internal/Internal Referred Object, which "
               "is still referred "
               "by Referrer object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_RESOURCE_IN_USE &&
         "Object Delete for Internal/Internal Referred Object should fail with "
         "resource in use "
         "error (but it did not), since it is still in use by referrer "
         "object!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_SUCCESS &&
         u_referred_attr_u64_val_out == u_referred_attr_u64_val_in &&
         "Attribute get failed for referred object!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_referred_oids_list, oids_val_out);
  assert(status == SWITCH_STATUS_SUCCESS &&
         oids_val_out.front() == referred_oid &&
         "Referrer object attribute referred oid get failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Referrer Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referrer object delete failed!");

  // Test Case 3: Now Delete the Referred Object. Should Succeed
  std::cout << "# Test Case 3: Delete Referred Object" << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Internal/Internal Referred Object delete failed!");
  std::cout << std::endl;
}

void test_auto_internal_user_reference() {
  std::cout << "**** Testing object delete when an Auto Object "
               "references "
               "a Internal User Object ****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{}, oid_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Creating Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  // Mark this object as Internal/Internal
  status = switch_store::v_set(
      referred_oid, SWITCH_USER_REFERRED_OBJECT_ATTR_INTERNAL_OBJECT, true);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Failed to mark User Referred Object as Internal/Internal!");

  // Create an User Object that refers above Referred object
  status = find_auto_oid(referred_oid, auto_referrer_object_type, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");

  // Test Case 1: Delete Referred Object. Should Succeed.
  std::cout << "# Test Case 1: Delete Internal/Internal Referred Object, which "
               "is still referred "
               "by Referrer object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Object Delete for Internal/Internal Referred Object failed!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND &&
         "Attribute get for Deleted Internal/Internal Referred User Object "
         "should return Item Not "
         "Found Error!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_referred_oid, oid_val_out);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND &&
         "Auto Referrer Object attribute get should fail with error Item Not "
         "Found after (Parent) Internal/Internal Referred User Object is "
         "deleted!");

  // Re Create referred object
  referred_oid = {};
  std::cout << "Recreating Referred Object" << std::endl;
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  // Mark this object as Internal/Internal
  status = switch_store::v_set(
      referred_oid, SWITCH_USER_REFERRED_OBJECT_ATTR_INTERNAL_OBJECT, true);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Failed to mark User Referred Object as Internal/Internal!");

  // Auto object that is child of this user object will automatically be created
  // by the factory infra
  status = find_auto_oid(referred_oid, auto_referrer_object_type, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Referrer Auto Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Referrer (Auto) object delete failed!");

  // Test Case 3: Now Delete the Referred Object. Should Succeed
  std::cout << "# Test Case 3: Delete Referred Object" << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Internal/Internal Referred (User) object delete failed!");
  std::cout << std::endl;
}

void test_user_internal_reference() {
  std::cout << "**** Testing object delete when a User Object Refers another "
               "User Object "
               "via an internal reference"
               " ****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{}, oid_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Creating Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  attrs.clear();

  // Create an Object that refers the above object via an internal reference
  std::cout << "Creating a User Object that refers above referred object via "
               "an internal reference"
            << std::endl;
  attr_w oid_attr(u_referrer_attr_internal_referred_oid);
  oid_attr.v_set(referred_oid);
  attrs.insert(oid_attr);
  status = switch_store::object_create(
      user_internal_reference_object_type, attrs, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");
  attrs.clear();

  // Test Case 1: Delete Referred Object. Should Succeed. Internal references
  // are ignored
  std::cout << "# Test Case 1: Delete Referred Object, which is referenced by "
               "an internal reference of Referrer Object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Object Delete for Referred Object should succeed even though it is "
         "being referenced by Referrer Object, since internal references are "
         "ignored!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND &&
         "Attribute get for Deleted Referred User Object should return Item "
         "Not Found Error!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_internal_referred_oid, oid_val_out);
  assert(status == SWITCH_STATUS_SUCCESS && oid_val_out == referred_oid &&
         "Referrer object attribute referred oid get failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Referrer Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referrer object delete failed!");
  std::cout << std::endl;
}

void test_user_read_only_reference() {
  std::cout << "**** Testing object delete when a User Object Refers another "
               "User Object "
               "via an internal reference"
               " ****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{}, oid_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Creating Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  attrs.clear();

  // Create an Object that refers the above object via a read only reference
  std::cout << "Creating a User Object that refers above referred object via a "
               "read only reference"
            << std::endl;
  attr_w oid_attr(u_referrer_attr_ro_referred_oid);
  oid_attr.v_set(referred_oid);
  attrs.insert(oid_attr);
  status = switch_store::object_create(
      user_read_only_reference_object_type, attrs, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");
  attrs.clear();

  // Test Case 1: Delete Referred Object. Should Succeed. Read only references
  // are ignored
  std::cout << "# Test Case 1: Delete Referred Object, which is referenced by "
               "a read only reference of Referrer Object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Object Delete for Referred Object should succeed even though it is "
         "being referenced by Referrer Object, since read only references are "
         "ignored!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND &&
         "Attribute get for Deleted Referred User Object should return Item "
         "Not Found Error!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_ro_referred_oid, oid_val_out);
  assert(status == SWITCH_STATUS_SUCCESS && oid_val_out == referred_oid &&
         "Referrer object attribute referred oid get failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Referrer Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referrer object delete failed!");
  std::cout << std::endl;
}

void test_user_read_only_internal_reference() {
  std::cout << "**** Testing object delete when a User Object Refers another "
               "User Object "
               "via an internal read_only reference"
               " ****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{}, oid_val_out{};
  uint64_t u_referred_attr_u64_val_in{1}, u_referred_attr_u64_val_out{};
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create referred object
  std::cout << "Creating Referred Object" << std::endl;
  attr_w u64_attr(u_referred_attr_u64);
  u64_attr.v_set(u_referred_attr_u64_val_in);
  attrs.insert(u64_attr);
  status = switch_store::object_create(
      user_referred_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  attrs.clear();

  // Create an Object that refers the above object via a read only reference
  std::cout << "Creating a User Object that refers above referred object via "
               "an internal read only reference"
            << std::endl;
  attr_w oid_attr(u_referrer_attr_internal_ro_referred_oid);
  oid_attr.v_set(referred_oid);
  attrs.insert(oid_attr);
  status = switch_store::object_create(
      user_internal_read_only_reference_object_type, attrs, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");
  attrs.clear();

  // Test Case 1: Delete Referred Object. Should Succeed. Internal Read only
  // references are ignored
  std::cout << "# Test Case 1: Delete Referred Object, which is referenced by "
               "an internal read only reference of Referrer Object"
            << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS &&
         "Object Delete for Referred Object should succeed even though it is "
         "being referenced by Referrer Object, since internal read only "
         "references are ignored!");
  status = switch_store::v_get(
      referred_oid, u_referred_attr_u64, u_referred_attr_u64_val_out);
  assert(status == SWITCH_STATUS_ITEM_NOT_FOUND &&
         "Attribute get for Deleted Referred User Object should return Item "
         "Not Found Error!");
  status = switch_store::v_get(
      referrer_oid, u_referrer_attr_internal_ro_referred_oid, oid_val_out);
  assert(status == SWITCH_STATUS_SUCCESS && oid_val_out == referred_oid &&
         "Referrer object attribute referred oid get failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Referrer Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referrer object delete failed!");
  std::cout << std::endl;
}

void test_user_user_reference_with_membership() {
  std::cout << "**** Testing object delete when one user object references "
               "another user object which subscribes as a member to another "
               "user object****"
            << std::endl;
  std::set<attr_w> attrs;
  switch_object_id_t referrer_oid{}, referred_oid{}, oid_val_out{}, group_oid{},
      group_oid_out{};
  std::vector<switch_object_id_t> members_out;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Create membership object
  std::cout << "Creating membership for Referred Object" << std::endl;
  status = switch_store::object_create(
      user_membership_object_type, attrs, group_oid);
  assert(status == SWITCH_STATUS_SUCCESS && group_oid.data != 0 &&
         "Membership Group Object Create Failed!");

  // Create referred object with membership to above object
  std::cout << "Create Referred Object" << std::endl;
  attr_w group_oid_attr(u_referred_membership_attr_member_of_oid);
  group_oid_attr.v_set(group_oid);
  attrs.insert(group_oid_attr);
  status = switch_store::object_create(
      user_referred_membership_object_type, attrs, referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referred_oid.data != 0 &&
         "Referred Object Create Failed!");
  attrs.clear();

  // Verify members of membership Object
  status =
      switch_store::v_get(group_oid, u_membership_attr_members, members_out);
  assert(status == SWITCH_STATUS_SUCCESS && members_out.size() == 1 &&
         members_out.front() == referred_oid &&
         "Membership Object members get failed");

  // Create referrer object that refers above referred object
  std::cout << "Create Referrer Object that refers above referred object"
            << std::endl;
  attr_w oid_attr(u_referrer_membership_attr_referred_oid);
  oid_attr.v_set(referred_oid);
  attrs.insert(oid_attr);
  status = switch_store::object_create(
      user_referrer_membership_object_type, attrs, referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && referrer_oid.data != 0 &&
         "Referrer Object Create Failed!");
  attrs.clear();

  // Test Case 1: Delete Referred Object. Should Return Object In Use Error.
  // Also neither the referred nor the referrer
  // object should be deleted from the database
  std::cout
      << "# Test Case 1.1: Delete Referred Object, which is still referred "
         "by Referrer object"
      << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_RESOURCE_IN_USE &&
         "Object Delete for Referred Object should fail with resource in use "
         "error (but it did not), since it is still in use be referrer "
         "object!");
  std::cout << "# Test Case 1.2: Verifying membership of Referred Object Post "
               "Delete Failure"
            << std::endl;
  members_out.clear();
  status =
      switch_store::v_get(group_oid, u_membership_attr_members, members_out);
  assert(status == SWITCH_STATUS_SUCCESS && members_out.size() == 1 &&
         members_out.front() == referred_oid &&
         "Membership check for referred Object Failed, Post Delete Failure!");
  std::cout << "# Test Case 1.3: Verifying attribute get for Referred Object "
               "Post Delete Failure"
            << std::endl;
  status = switch_store::v_get(
      referred_oid, u_referred_membership_attr_member_of_oid, group_oid_out);
  assert(status == SWITCH_STATUS_SUCCESS && group_oid_out == group_oid &&
         "Attribute get failed for referred object!");
  std::cout << "# Test Case 1.4: Verifying attribute get for Referrer Object "
               "Post Referred Object Delete Failure"
            << std::endl;
  status = switch_store::v_get(
      referrer_oid, u_referrer_membership_attr_referred_oid, oid_val_out);
  assert(status == SWITCH_STATUS_SUCCESS && oid_val_out == referred_oid &&
         "Referrer object attribute referred oid get failed!");

  // Test Case 2: Now Delete the Referrer Object. Should Succeed
  std::cout << "# Test Case 2: Delete Referrer Object" << std::endl;
  status = switch_store::object_delete(referrer_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referrer object delete failed!");

  // Test Case 3: Now Delete the Referred Object. Should Succeed
  std::cout << "# Test Case 3: Delete Referred Object" << std::endl;
  status = switch_store::object_delete(referred_oid);
  assert(status == SWITCH_STATUS_SUCCESS && "Referred object delete failed!");
  std::cout << std::endl;
}

int main(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  status = switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);

  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  init_objects();
  test_user_user_reference();
  test_user_user_list_reference();
  test_auto_user_reference();
  // test_user_auto_reference();
  test_internal_user_user_reference();
  test_internal_user_user_list_reference();
  test_user_internal_user_reference();
  test_user_internal_user_list_reference();
  test_auto_internal_user_reference();
  test_user_internal_reference();
  test_user_read_only_reference();
  test_user_read_only_internal_reference();
  // Uncomment this when object delete failure is fixed for objects having
  // membership
  test_user_user_reference_with_membership();

  test_internal_user_user_reference_with_triggers();

  std::cout << "All tests passed!\n";
  return 0;
}
