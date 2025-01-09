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
#include "s3/attribute_util.h"
#include "bf_switch/bf_event.h"
#include "s3/switch_store.h"
#include "s3/factory.h"
#include "s3/event.h"
#include "../log.h"

using namespace smi;
using namespace std;
using namespace bf_switch;

ModelInfo *model_info = NULL;

class test_auto_1 : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_1;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_1_ATTR_PARENT_HANDLE;
  switch_mac_addr_t mac;
  switch_mac_payload_t payload = {};
  switch_mac_event_data_t mac_data;

 public:
  test_auto_1(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status =
        switch_store::v_get(parent, SWITCH_TEST_OBJECT_4_ATTR_TEST_MAC, mac);
    payload.mac_event = SWITCH_MAC_EVENT_MOVE;
    payload.mac_handle = parent;
    mac_data.payload.push_back(payload);
  }
  switch_status_t create_update() {
    event::mac_event_notify(mac_data);
    auto_object::create_update();
    return SWITCH_STATUS_SUCCESS;
  }
  switch_status_t del() {
    auto_object::del();
    return SWITCH_STATUS_SUCCESS;
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
    (void)status;
  }
  switch_status_t create_update() {
    event::port_event_notify(SWITCH_PORT_EVENT_ADD, get_parent());
    event::port_status_notify(SWITCH_PORT_OPER_STATUS_UP, get_parent());
    auto_object::create_update();
    return SWITCH_STATUS_SUCCESS;
  }
  switch_status_t del() {
    auto_object::del();
    return SWITCH_STATUS_SUCCESS;
  }
};

#ifdef __cplusplus
extern "C" {
#endif

void test_mac_events() {
  std::cout << "**** Tesing MAC creation ****" << std::endl;
  switch_status_t status;
  REGISTER_OBJECT(test_auto_1, SWITCH_OBJECT_TYPE_TEST_AUTO_1);
  REGISTER_OBJECT(test_auto_2, SWITCH_OBJECT_TYPE_TEST_AUTO_2);

  switch_mac_addr_t b_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  attr_w mac_attr = attr_w(SWITCH_TEST_OBJECT_4_ATTR_TEST_MAC, b_mac);

  const std::set<attr_w> attrs{mac_attr};
  switch_object_id_t oid = {};

  /* create */
  status =
      switch_store::object_create(SWITCH_OBJECT_TYPE_TEST_OBJECT_4, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  /* delete */
  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_events() {
  std::cout << "**** Tesing objects creation ****" << std::endl;
  switch_status_t status;
  REGISTER_OBJECT(test_auto_1, SWITCH_OBJECT_TYPE_TEST_AUTO_1);
  REGISTER_OBJECT(test_auto_2, SWITCH_OBJECT_TYPE_TEST_AUTO_2);

  const std::set<attr_w> attrs2{};
  switch_object_id_t oid1 = {}, oid2 = {};

  bf_switch_object_event_notify_set(SWITCH_OBJECT_TYPE_TEST_OBJECT_1,
                                    SWITCH_OBJECT_EVENT_CREATE);
  bf_switch_object_event_notify_set(SWITCH_OBJECT_TYPE_TEST_OBJECT_1,
                                    SWITCH_OBJECT_EVENT_DELETE);
  bf_switch_object_event_notify_set(SWITCH_OBJECT_TYPE_TEST_OBJECT_2,
                                    SWITCH_OBJECT_EVENT_CREATE);

  /* create */
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_TEST_OBJECT_2, attrs2, oid2);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid2);
  assert(status == SWITCH_STATUS_SUCCESS);

  /* create */
  status = switch_store::object_create(
      SWITCH_OBJECT_TYPE_TEST_OBJECT_1, attrs2, oid1);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::attribute_set(
      oid1, attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_OID, oid2));
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_delete(oid1);
  assert(status == SWITCH_STATUS_SUCCESS);

  bf_switch_object_event_notify_all(SWITCH_OBJECT_TYPE_TEST_OBJECT_2);
}

void init_events() {
  const char *const test_model_name = TESTDATADIR "/test/test_model.json";
  switch_store::object_info_init(test_model_name, false, NULL);
  event::event_init();
}

#ifdef __cplusplus
}
#endif
