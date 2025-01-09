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

class direct_stats : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_DIRECT_STATS;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_DIRECT_STATS_ATTR_PARENT_HANDLE;

 public:
  direct_stats(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }
  switch_status_t counters_get(switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntr) {
    (void)handle;
    (void)cntr;
    return SWITCH_STATUS_SUCCESS;
  }
  switch_status_t counters_set(switch_object_id_t handle) {
    (void)handle;
    return SWITCH_STATUS_SUCCESS;
  }
};
class indirect_stats_1 : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INDIRECT_STATS_1;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INDIRECT_STATS_1_ATTR_PARENT_HANDLE;

 public:
  indirect_stats_1(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }
  switch_status_t counters_get(switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntr) {
    (void)handle;
    (void)cntr;
    switch_object_id_t counter_handle = {0};
    switch_store::v_get(
        handle, SWITCH_STATS_2_ATTR_COUNTER_HANDLE, counter_handle);
    if (counter_handle.data == 0)
      return SWITCH_STATUS_FAILURE;
    else
      return SWITCH_STATUS_SUCCESS;
  }
};
class indirect_stats_2 : public auto_object {
 private:
  static const switch_object_type_t auto_ot =
      SWITCH_OBJECT_TYPE_INDIRECT_STATS_2;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_INDIRECT_STATS_2_ATTR_PARENT_HANDLE;

 public:
  indirect_stats_2(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }
  switch_status_t counters_get(switch_object_id_t handle,
                               std::vector<switch_counter_t> &cntr) {
    (void)handle;
    (void)cntr;
    switch_object_id_t counter_handle = {0};
    switch_store::v_get(
        handle, SWITCH_STATS_2_ATTR_COUNTER_HANDLE, counter_handle);
    if (counter_handle.data == 0)
      return SWITCH_STATUS_FAILURE;
    else
      return SWITCH_STATUS_SUCCESS;
  }
};
class stats_dep : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_STATS_DEP;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_STATS_DEP_ATTR_PARENT_HANDLE;

 public:
  stats_dep(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
  }
};

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
  status = switch_store::object_create(stats_1, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  std::vector<switch_counter_t> cattrs{};
  status = switch_store::object_counters_get(oid, cattrs);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_counters_clear_all(oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_counter_creation_2() {
  std::cout << "\n**** Tesing indirect stats ****" << std::endl;
  switch_status_t status;

  const ObjectInfo *counter_info =
      model_info->get_object_info_from_name("counter");
  switch_object_type_t counter = counter_info->object_type;
  assert(counter != 0);
  const ObjectInfo *stats2_info =
      model_info->get_object_info_from_name("stats_2");
  switch_object_type_t stats_2 = stats2_info->object_type;
  assert(stats_2 != 0);
  switch_attr_id_t cntr_attr_id =
      stats2_info->get_attr_id_from_name("counter_handle");

  const std::set<attr_w> attrs{};
  std::set<attr_w> cattrs{};
  switch_object_id_t oid = {}, cid = {};

  /* create */
  status = switch_store::object_create(stats_2, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_create(counter, cattrs, cid);
  assert(status == SWITCH_STATUS_SUCCESS);

  std::vector<switch_counter_t> cntrs{};
  status = switch_store::object_counters_get(oid, cntrs);
  assert(status == SWITCH_STATUS_FAILURE);

  smi::attr_w attr(cntr_attr_id, cid);
  status = switch_store::attribute_set(oid, attr);
  assert(status == SWITCH_STATUS_SUCCESS);

  cattrs.clear();
  status = switch_store::object_counters_get(oid, cntrs);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(cid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

int main(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const char *const test_model_name = TESTDATADIR "/test/test_model.json";
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);

  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  REGISTER_OBJECT(direct_stats, SWITCH_OBJECT_TYPE_DIRECT_STATS);
  REGISTER_OBJECT(indirect_stats_1, SWITCH_OBJECT_TYPE_INDIRECT_STATS_1);
  REGISTER_OBJECT(indirect_stats_2, SWITCH_OBJECT_TYPE_INDIRECT_STATS_2);
  REGISTER_OBJECT(stats_dep, SWITCH_OBJECT_TYPE_STATS_DEP);

  test_counter_creation();
  test_counter_creation_2();

  printf("\nAll tests passed!\n");
  return 0;
}
