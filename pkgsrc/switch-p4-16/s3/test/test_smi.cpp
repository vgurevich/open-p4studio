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
#include "gen-model/test_model.h"
#include "s3/attribute.h"
#include "s3/attribute_util.h"
#include "s3/switch_store.h"
#include "s3/factory.h"
#include "s3/smi.h"

using namespace smi;

static ModelInfo *model_info = NULL;
static switch_object_type_t test_object_7;
const ObjectInfo *test_object_7_info;

static switch_object_type_t test_object_9;

static switch_attr_id_t obj_7_key_1_attr;
static switch_attr_id_t obj_7_key_2_attr;
static switch_attr_id_t obj_7_key_3_attr;

void init_objects() {
  test_object_7_info = model_info->get_object_info_from_name("test_object_7");
  test_object_7 = test_object_7_info->object_type;
  assert(test_object_7 != 0);
  obj_7_key_1_attr = test_object_7_info->get_attr_id_from_name("test_uint8");
  assert(obj_7_key_1_attr != 0);
  obj_7_key_2_attr = test_object_7_info->get_attr_id_from_name("test_uint64");
  assert(obj_7_key_2_attr != 0);
  obj_7_key_3_attr = test_object_7_info->get_attr_id_from_name("test_uint16");
  assert(obj_7_key_3_attr != 0);
  test_object_9 = 100;
}

void test_object() {
  switch_status_t status;
  uint8_t obj7_1_8 = 8;
  uint64_t obj7_1_64 = 64;
  uint16_t obj7_1_16 = 16;

  std::set<attr_w> mand_attrs{attr_w(obj_7_key_1_attr, obj7_1_8)};
  std::set<attr_w> attrs{attr_w(obj_7_key_1_attr, obj7_1_8)};
  switch_object_id_t oid_7 = {};
  switch_object_id_t oid_9 = {};
  status = smi::api::smi_object_create(test_object_7, mand_attrs, oid_7);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  mand_attrs.insert(attr_w(obj_7_key_2_attr, obj7_1_64));
  attrs.insert(attr_w(obj_7_key_2_attr, obj7_1_64));
  status = smi::api::smi_object_create(test_object_7, mand_attrs, oid_7);
  assert(status == SWITCH_STATUS_SUCCESS);

  std::vector<switch_object_id_t> obj_handle;
  obj_handle.push_back(oid_7);
  status =
      smi::api::smi_log_level_set(switch_verbosity_t::SWITCH_API_LEVEL_INFO);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = smi::api::smi_object_get_all_handles(test_object_7, obj_handle);
  assert(status == SWITCH_STATUS_SUCCESS);

  obj7_1_8 = 17;
  status =
      smi::api::smi_attribute_set(oid_7, attr_w(obj_7_key_1_attr, obj7_1_8));
  assert(status == SWITCH_STATUS_SUCCESS);

  obj7_1_16 = 8;
  status =
      smi::api::smi_attribute_set(oid_7, attr_w(obj_7_key_3_attr, obj7_1_16));
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  attr_w attr(obj_7_key_3_attr);
  status = smi::api::smi_attribute_get(oid_7, obj_7_key_3_attr, attr);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = smi::api::smi_object_flush_all(test_object_7);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = smi::api::smi_object_create(test_object_7, mand_attrs, oid_7);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = smi::api::smi_object_log_level_set(
      test_object_7, switch_verbosity_t::SWITCH_API_LEVEL_DEBUG);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = smi::api::smi_object_create(test_object_9, mand_attrs, oid_9);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  mand_attrs.insert(attr_w(obj_7_key_3_attr, obj7_1_16));
  status = smi::api::smi_object_create(test_object_7, mand_attrs, oid_7);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_store::object_delete(oid_7);
  assert(status == SWITCH_STATUS_SUCCESS);

  uint32_t id = 7;
  status = smi::api::smi_object_create_by_id(test_object_7, attrs, id);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = smi::api::smi_object_create_by_id(test_object_9, attrs, id);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);
  bool exists = smi::api::smi_object_exists_by_id(test_object_7, id);
  assert(exists == true);

  bool not_exists = smi::api::smi_object_exists_by_id(test_object_9, id);
  assert(not_exists == false);

  status = smi::api::smi_object_delete_by_id(test_object_7, id);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = smi::api::smi_object_delete_by_id(test_object_9, id);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);
}

void test_counter_creation() {
  std::cout << "\n**** Tesing direct stats ****" << std::endl;
  switch_status_t status;

  const ObjectInfo *counter_info =
      model_info->get_object_info_from_name("counter");
  switch_object_type_t counter = counter_info->object_type;
  assert(counter != 0);
  const ObjectInfo *stats1_info =
      model_info->get_object_info_from_name("stats_1");
  switch_object_type_t stats_1 = stats1_info->object_type;
  assert(stats_1 != 0);
  const std::set<attr_w> attrs{};
  switch_object_id_t oid = {};

  /* create */
  status = smi::api::smi_object_create(stats_1, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  std::vector<switch_counter_t> cattrs{};
  status = smi::api::smi_object_counters_get(oid, cattrs);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = smi::api::smi_object_counters_clear_all(oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}
int main(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const char *const test_model_name = TESTDATADIR "/test/test_model.json";
  status = switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);

  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  init_objects();
  test_object();
  test_counter_creation();

  printf("\n\nAll tests passed!\n");
  return 0;
}