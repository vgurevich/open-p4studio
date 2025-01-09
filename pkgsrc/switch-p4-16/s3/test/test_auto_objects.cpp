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

#include "gen-model/test_model.h"
#include "bf_switch/bf_switch_types.h"
#include "s3/attribute.h"
#include "s3/factory.h"
#include "s3/switch_store.h"

using namespace smi;
ModelInfo *model_info = NULL;

class test_auto_1 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_1;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_1_ATTR_PARENT_HANDLE;

 public:
  test_auto_1(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    switch_object_id_t auto_2_oid;
    find_auto_oid(parent, SWITCH_OBJECT_TYPE_TEST_AUTO_2, auto_2_oid);
    if (auto_2_oid.data == 0) status = SWITCH_STATUS_DEPENDENCY_FAILURE;
  }
};
class test_auto_2 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_2;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_2_ATTR_PARENT_HANDLE;

 public:
  test_auto_2(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }
};
class test_auto_3 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_3;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_3_ATTR_PARENT_HANDLE;

 public:
  test_auto_3(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }
};
class test_auto_4 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_4;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_4_ATTR_PARENT_HANDLE;

 public:
  test_auto_4(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }
  switch_status_t create_update() { return SWITCH_STATUS_INVALID_PARAMETER; }
};
class test_auto_5 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_5;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_5_ATTR_PARENT_HANDLE;

 public:
  test_auto_5(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_ITEM_ALREADY_EXISTS;
  }
};
class test_auto_6 : public object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_6;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_6_ATTR_PARENT_HANDLE;

 public:
  test_auto_6(const switch_object_id_t parent, switch_status_t &status) {
    (void)status;
    (void)parent;
  }
  switch_status_t create_update() { return SWITCH_STATUS_SUCCESS; }
  switch_status_t del() { return SWITCH_STATUS_SUCCESS; }
};
class test_auto_62 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_6;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_6_ATTR_PARENT_HANDLE;

 public:
  test_auto_62(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    uint64_t u64 = 0;
    status =
        switch_store::v_get(parent, SWITCH_TEST_OBJECT_4_ATTR_TEST_UINT64, u64);
    if (u64 > 100) status = SWITCH_STATUS_INVALID_PARAMETER;
  }
};

// test auto object creation priority
class test_prio_1 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_PRIO_1;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_PRIO_1_ATTR_PARENT_HANDLE;

 public:
  test_prio_1(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    (void)status;
    switch_object_id_t prio_3_oid, prio_4_oid;
    find_auto_oid(parent, SWITCH_OBJECT_TYPE_TEST_PRIO_3, prio_3_oid);
    find_auto_oid(parent, SWITCH_OBJECT_TYPE_TEST_PRIO_4, prio_4_oid);
    assert(prio_3_oid.data != 0);
    assert(prio_4_oid.data != 0);
  }
};
class test_prio_2 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_PRIO_2;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_PRIO_2_ATTR_PARENT_HANDLE;

 public:
  test_prio_2(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    (void)status;
    switch_object_id_t prio_3_oid, prio_4_oid;
    find_auto_oid(parent, SWITCH_OBJECT_TYPE_TEST_PRIO_3, prio_3_oid);
    find_auto_oid(parent, SWITCH_OBJECT_TYPE_TEST_PRIO_4, prio_4_oid);
    assert(prio_3_oid.data != 0);
    assert(prio_4_oid.data != 0);
  }
};
class test_prio_3 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_PRIO_3;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_PRIO_3_ATTR_PARENT_HANDLE;

 public:
  test_prio_3(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    (void)status;
    switch_object_id_t prio_4_oid;
    find_auto_oid(parent, SWITCH_OBJECT_TYPE_TEST_PRIO_4, prio_4_oid);
    assert(prio_4_oid.data != 0);
  }
};
class test_prio_4 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_PRIO_4;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_PRIO_4_ATTR_PARENT_HANDLE;

 public:
  test_prio_4(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }
};
void test_object_creation() {
  std::cout << "**** Tesing objects creation ****" << std::endl;
  switch_status_t status;

  REGISTER_OBJECT(test_auto_1, SWITCH_OBJECT_TYPE_TEST_AUTO_1);
  REGISTER_OBJECT(test_auto_2, SWITCH_OBJECT_TYPE_TEST_AUTO_2);
  REGISTER_OBJECT(test_auto_3, SWITCH_OBJECT_TYPE_TEST_AUTO_3);
  REGISTER_OBJECT(test_auto_6, SWITCH_OBJECT_TYPE_TEST_AUTO_6);
  {
    const ObjectInfo *object_info =
        model_info->get_object_info_from_name("test_object_4");
    switch_object_type_t test_object_4 = object_info->object_type;
    assert(test_object_4 != 0);
    const std::set<attr_w> attrs{};
    switch_object_id_t oid = {};

    /* create */
    status = switch_store::object_create(test_object_4, attrs, oid);
    assert(status == SWITCH_STATUS_SUCCESS);

    status = switch_store::object_delete(oid);
    assert(status == SWITCH_STATUS_SUCCESS);
  }

  REGISTER_OBJECT(test_prio_1, SWITCH_OBJECT_TYPE_TEST_PRIO_1);
  REGISTER_OBJECT(test_prio_2, SWITCH_OBJECT_TYPE_TEST_PRIO_2);
  REGISTER_OBJECT(test_prio_3, SWITCH_OBJECT_TYPE_TEST_PRIO_3);
  REGISTER_OBJECT(test_prio_4, SWITCH_OBJECT_TYPE_TEST_PRIO_4);
  {
    const ObjectInfo *object_info =
        model_info->get_object_info_from_name("test_object_8");
    switch_object_type_t test_object_8 = object_info->object_type;
    assert(test_object_8 != 0);
    const std::set<attr_w> attrs{};
    switch_object_id_t oid = {};

    /* create */
    status = switch_store::object_create(test_object_8, attrs, oid);
    assert(status == SWITCH_STATUS_SUCCESS);

    status = switch_store::object_delete(oid);
    assert(status == SWITCH_STATUS_SUCCESS);
  }

  factory_clean();
}

/* test_auto_4 returns failure in create_update */
void test_object_failure_1() {
  std::cout << "**** Tesing object create update faiure 1 ****" << std::endl;
  switch_status_t status;

  REGISTER_OBJECT(test_auto_2, SWITCH_OBJECT_TYPE_TEST_AUTO_2);
  REGISTER_OBJECT(test_auto_3, SWITCH_OBJECT_TYPE_TEST_AUTO_3);
  REGISTER_OBJECT(test_auto_4, SWITCH_OBJECT_TYPE_TEST_AUTO_4);

  const ObjectInfo *object_info =
      model_info->get_object_info_from_name("test_object_5");
  switch_object_type_t test_object_5 = object_info->object_type;
  assert(test_object_5 != 0);
  const std::set<attr_w> attrs{};
  switch_object_id_t oid = {0};

  /* create */
  status = switch_store::object_create(test_object_5, attrs, oid);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);
  assert(oid.data == 0);

  factory_clean();
}

/* test_auto_5 returns failure in constructor */
void test_object_failure_2() {
  std::cout << "**** Tesing object creation faiure 2 ****" << std::endl;
  switch_status_t status;

  REGISTER_OBJECT(test_auto_2, SWITCH_OBJECT_TYPE_TEST_AUTO_2);
  REGISTER_OBJECT(test_auto_3, SWITCH_OBJECT_TYPE_TEST_AUTO_3);
  REGISTER_OBJECT(test_auto_5, SWITCH_OBJECT_TYPE_TEST_AUTO_5);

  const ObjectInfo *object_info =
      model_info->get_object_info_from_name("test_object_6");
  switch_object_type_t test_object_6 = object_info->object_type;
  assert(test_object_6 != 0);
  const std::set<attr_w> attrs{};
  switch_object_id_t oid = {0};

  /* create */
  status = switch_store::object_create(test_object_6, attrs, oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);
  assert(oid.data == 0);

  factory_clean();
}

/* test_auto_62 returns error for various attribute values */
void test_object_update() {
  std::cout << "**** Tesing objects update ****" << std::endl;
  switch_status_t status;

  REGISTER_OBJECT(test_auto_62, SWITCH_OBJECT_TYPE_TEST_AUTO_6);
  {
    const ObjectInfo *object_info =
        model_info->get_object_info_from_name("test_object_4");
    switch_object_type_t test_object_4 = object_info->object_type;
    assert(test_object_4 != 0);
    switch_attr_id_t test_object_4_test_uint64 =
        object_info->get_attr_id_from_name("test_uint64");
    const std::set<attr_w> attrs{};
    switch_object_id_t oid = {};

    /* create */
    status = switch_store::object_create(test_object_4, attrs, oid);
    assert(status == SWITCH_STATUS_SUCCESS);

    uint64_t val = 0;
    {
      attr_w attr(test_object_4_test_uint64);
      status =
          switch_store::attribute_get(oid, test_object_4_test_uint64, attr);
      attr.v_get(val);
      assert(val == 0);
    }
    {
      // basic validation for valid value
      val = 99;
      attr_w set_attr(test_object_4_test_uint64, val);
      attr_w get_attr(test_object_4_test_uint64);
      status = switch_store::attribute_set(oid, set_attr);
      assert(status == SWITCH_STATUS_SUCCESS);
      status =
          switch_store::attribute_get(oid, test_object_4_test_uint64, get_attr);
      val = 0;
      get_attr.v_get(val);
      assert(val == 99);
    }
    {
      // store should not be updated if auto_object fails
      val = 101;
      attr_w set_attr(test_object_4_test_uint64, val);
      attr_w get_attr(test_object_4_test_uint64);
      status = switch_store::attribute_set(oid, set_attr);
      assert(status == SWITCH_STATUS_INVALID_PARAMETER);
      // db cannot change on failure
      status =
          switch_store::attribute_get(oid, test_object_4_test_uint64, get_attr);
      val = 0;
      get_attr.v_get(val);
      assert(val == 99);
      // future calls should succeed
      val = 98;
      set_attr.v_set(val);
      status = switch_store::attribute_set(oid, set_attr);
      assert(status == SWITCH_STATUS_SUCCESS);
      status =
          switch_store::attribute_get(oid, test_object_4_test_uint64, get_attr);
      val = 0;
      get_attr.v_get(val);
      std::cout << val << std::endl;
      assert(val == 98);
    }

    status = switch_store::object_delete(oid);
    assert(status == SWITCH_STATUS_SUCCESS);
  }

  factory_clean();
}

int main(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const char *const test_model_name = TESTDATADIR "/test/test_model.json";
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);

  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  test_object_creation();
  test_object_failure_1();
  test_object_failure_2();
  test_object_update();

  printf("\n\nAll tests passed!\n");
  return 0;
}
