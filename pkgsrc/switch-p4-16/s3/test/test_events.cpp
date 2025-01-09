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
#include "test_events.h"

using namespace smi;
using namespace std;
using namespace bf_switch;

static int notif_count = 0;

void switch_port_status_event_cb(switch_port_oper_status_event_data_t data) {
  assert(data.port_status_event == SWITCH_PORT_OPER_STATUS_UP);
  notif_count++;
}

void switch_port_event_cb(switch_port_event_data_t data) {
  assert(data.port_event == SWITCH_PORT_EVENT_ADD);
  notif_count++;
}

void switch_mac_event_cb(switch_mac_event_data_t data) {
  assert(data.payload.size() == 1);
  assert(data.payload[0].mac_event == SWITCH_MAC_EVENT_MOVE);
  notif_count++;
}

void switch_mac_event_cb_c(switch_mac_event_data_c_t *data) {
  assert(data->count == 1);
  assert(data->payload[0].mac_event == SWITCH_MAC_EVENT_MOVE);
  notif_count++;
  free(data->payload);
  free(data);
}

void switch_object_event_cb(switch_object_event_data_t data) {
  cout << data.event << endl;
  cout << data.object_type << endl;
  cout << data.object_id << endl;
  cout << data.status << endl;
  cout << data.attr.id << endl;
  notif_count++;
}

int main(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const char *const test_model_name = TESTDATADIR "/test/test_model.json";
  switch_store::object_info_init(test_model_name, false, NULL);
  event::event_init();

  assert(status == SWITCH_STATUS_SUCCESS);

  bf_switch_event_register(SWITCH_OBJECT_EVENT,
                           (void *)&switch_object_event_cb);
  bf_switch_event_register(SWITCH_MAC_EVENT, (void *)&switch_mac_event_cb);
  bf_switch_event_register(SWITCH_PORT_EVENT, (void *)&switch_port_event_cb);
  bf_switch_event_register(SWITCH_PORT_OPER_STATUS_EVENT,
                           (void *)&switch_port_status_event_cb);

  test_object_events();
  // Check for 2 creates and and 1 delete
  assert(notif_count == 3);
  bf_switch_event_deregister(SWITCH_OBJECT_EVENT);
  notif_count = 0;

  test_mac_events();
  // 1 each of mac, port and status events
  assert(notif_count == 3);

  bf_switch_event_register_c(SWITCH_MAC_EVENT, (void *)&switch_mac_event_cb_c);
  test_mac_events();
  // 1 each of mac, port and status events
  assert(notif_count == 6);

  bf_switch_event_deregister(SWITCH_OBJECT_EVENT);
  bf_switch_event_deregister(SWITCH_MAC_EVENT);
  bf_switch_event_deregister(SWITCH_PORT_EVENT);
  bf_switch_event_deregister(SWITCH_PORT_OPER_STATUS_EVENT);

  printf("\n\nAll tests passed!\n");
  return 0;
}
