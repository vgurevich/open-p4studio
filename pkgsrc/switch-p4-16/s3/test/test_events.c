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

#include "assert.h"
#include "stdio.h"
#include "stdlib.h"

#include "gen-model/test_model.h"
#include "bf_switch/bf_switch_types.h"
#include "bf_switch/bf_event.h"
#include "test_events.h"

static int notif_count = 0;

void switch_mac_event_cb_c(switch_mac_event_data_c_t *data) {
  assert(data->count == 1);
  assert(data->payload[0].mac_event == SWITCH_MAC_EVENT_MOVE);
  notif_count++;
  free(data->payload);
  free(data);
}

int main(void) {
  init_events();

  bf_switch_event_register_c(SWITCH_MAC_EVENT, (void *)&switch_mac_event_cb_c);
  test_mac_events();
  // 1 each of mac, port and status events
  assert(notif_count == 1);

  printf("\n\nAll tests passed!\n");
  return 0;
}
