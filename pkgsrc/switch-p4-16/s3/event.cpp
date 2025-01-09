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


#include "s3/event.h"

#include <stdio.h>
#include <string.h>

#include <unordered_map>
#include <sstream>

#include "s3/switch_store.h"
#include "s3/record.h"
#include "./log.h"

std::ostream &operator<<(std::ostream &os, const switch_object_event_t &event) {
  switch (event) {
    case SWITCH_OBJECT_EVENT_NONE:
      os << "SWITCH_OBJECT_EVENT_NONE";
      break;
    case SWITCH_OBJECT_EVENT_CREATE:
      os << "SWITCH_OBJECT_EVENT_CREATE";
      break;
    case SWITCH_OBJECT_EVENT_DELETE:
      os << "SWITCH_OBJECT_EVENT_DELETE";
      break;
    case SWITCH_OBJECT_EVENT_SET:
      os << "SWITCH_OBJECT_EVENT_SET";
      break;
    case SWITCH_OBJECT_EVENT_GET:
      os << "SWITCH_OBJECT_EVENT_GET";
      break;
  }
  return os;
}

namespace smi {
namespace event {
using ::smi::logging::switch_log;

std::unordered_map<switch_object_type_t, int> ObjectEventMap;
static smi_event_cb_t notification;

switch_status_t event_init() {
  ModelInfo *model_info = switch_store::switch_model_info_get();

  /* Initialize the callback structure */
  memset(&notification, 0, sizeof(smi_event_cb_t));

  /* Set default events for all objects to NONE */
  for (auto it = model_info->begin(); it != model_info->end(); it++) {
    auto object_info = *it;
    ObjectEventMap[object_info.object_type] = SWITCH_OBJECT_EVENT_NONE;
  }

  return SWITCH_STATUS_SUCCESS;
}

void demux_mac_callback(const switch_mac_event_data_t &data) {
  int i = 0;
  switch_mac_event_data_c_t *data_c = static_cast<switch_mac_event_data_c_t *>(
      calloc(1, sizeof(switch_mac_event_data_c_t)));
  if (data_c == NULL) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "{}.{}: Failed to allocate memory for MAC data",
               __func__,
               __LINE__);
    return;
  }
  data_c->count = data.payload.size();
  data_c->payload = static_cast<switch_mac_payload_t *>(
      calloc(data.payload.size(), sizeof(switch_mac_payload_t)));
  if (data_c->payload == NULL) {
    switch_log(SWITCH_API_LEVEL_ERROR,
               SWITCH_OT_NONE,
               "{}.{}: Failed to allocate memory for MAC data payload size {}",
               __func__,
               __LINE__,
               data.payload.size());
    return;
  }

  for (const switch_mac_payload_t &payload : data.payload) {
    data_c->payload[i].mac_event = payload.mac_event;
    data_c->payload[i].mac_handle = payload.mac_handle;
    i++;
  }
  if (notification.mac_event_c) notification.mac_event_c(data_c);
}

// This just piggybacks on the C++ data. Any C application will register with
// mac_event_c and the original C++ callback just invokes the C callback after
// converting from vector to array
void override_mac_callback_for_c(void *cb) {
  notification.mac_event = (smi_mac_event_cb)demux_mac_callback;
  notification.mac_event_c = (smi_mac_event_cb_c)cb;
}

void object_event_register(switch_event_t event, void *cb) {
  switch (event) {
    case SWITCH_OBJECT_EVENT:
      notification.object_event = (smi_object_event_cb)cb;
      break;
    case SWITCH_MAC_EVENT:
      notification.mac_event = (smi_mac_event_cb)cb;
      break;
    case SWITCH_NAT_EVENT:
      notification.nat_event = (smi_nat_event_cb)cb;
      break;
    case SWITCH_PORT_EVENT:
      notification.port_event = (smi_port_event_cb)cb;
      break;
    case SWITCH_PORT_OPER_STATUS_EVENT:
      notification.port_oper_status_event = (smi_port_oper_status_event_cb)cb;
      break;
    case SWITCH_PACKET_EVENT:
      notification.packet_event = (smi_packet_event_cb)cb;
      break;
    case SWITCH_DEVICE_EVENT:
      notification.device_event = (smi_device_oper_status_event_cb)cb;
      break;
    case SWITCH_BFD_EVENT:
      notification.bfd_event = (smi_bfd_event_cb)cb;
      break;
  }
}

void object_event_deregister(switch_event_t event) {
  switch (event) {
    case SWITCH_OBJECT_EVENT:
      notification.object_event = NULL;
      break;
    case SWITCH_MAC_EVENT:
      notification.mac_event = NULL;
      notification.mac_event_c = NULL;
      break;
    case SWITCH_NAT_EVENT:
      notification.nat_event = NULL;
      notification.nat_event_c = NULL;
      break;
    case SWITCH_PORT_EVENT:
      notification.port_event = NULL;
      break;
    case SWITCH_PORT_OPER_STATUS_EVENT:
      notification.port_oper_status_event = NULL;
      break;
    case SWITCH_PACKET_EVENT:
      notification.packet_event = NULL;
      break;
    case SWITCH_DEVICE_EVENT:
      notification.device_event = NULL;
      break;
    case SWITCH_BFD_EVENT:
      notification.bfd_event = NULL;
      break;
  }
}

void object_event_notify_set(const switch_object_type_t obj_type,
                             const switch_object_event_t event) {
  ObjectEventMap[obj_type] |= event;
}

void object_event_notify_set_all(const switch_object_type_t obj_type) {
  ObjectEventMap[obj_type] |=
      (SWITCH_OBJECT_EVENT_CREATE | SWITCH_OBJECT_EVENT_DELETE |
       SWITCH_OBJECT_EVENT_SET | SWITCH_OBJECT_EVENT_GET);
}

void object_notify(const switch_object_event_t event,
                   const switch_object_type_t object_type,
                   const switch_object_id_t object_id,
                   const switch_attribute_t attr,
                   const switch_status_t status) {
  /* Return if event not needed for this object_type */
  if (!(ObjectEventMap[object_type] & event)) return;

  switch_object_event_data_t data;
  memset(&data, 0, sizeof(switch_object_event_data_t));

  data.event = event;
  data.object_type = object_type;
  data.object_id = object_id;
  memcpy(&data.attr, &attr, sizeof(attr));
  data.status = status;

  if (notification.object_event) notification.object_event(data);
  return;
}

void port_status_notify(const switch_port_oper_status_event_t oper_status,
                        const switch_object_id_t port_handle) {
  switch_port_oper_status_event_data_t data;
  memset(&data, 0, sizeof(switch_port_oper_status_event_data_t));

  data.port_status_event = oper_status;
  data.object_id = port_handle;

  std::stringstream ss;
  ss << "port_status:" << port_handle << "|" << oper_status;
  smi::record::record_add_notify(ss.str());

  if (notification.port_oper_status_event)
    notification.port_oper_status_event(data);
  return;
}

void port_event_notify(const switch_port_event_t add,
                       const switch_object_id_t port_handle) {
  switch_port_event_data_t data;
  memset(&data, 0, sizeof(switch_port_event_data_t));

  data.port_event = add;
  data.object_id = port_handle;

  std::stringstream ss;
  ss << "port_event:" << port_handle << "|" << add;
  smi::record::record_add_notify(ss.str());

  if (notification.port_event) notification.port_event(data);
  return;
}

void mac_event_notify(const switch_mac_event_data_t &mac_data) {
  std::stringstream ss;
  for (auto &pyld : mac_data.payload) {
    ss << "mac_event:" << pyld.mac_handle << "|" << pyld.mac_event << "|"
       << pyld.port_lag_handle;
    smi::record::record_add_notify(ss.str());
  }
  if (notification.mac_event) notification.mac_event(mac_data);
  return;
}

void nat_event_notify(const switch_nat_event_data_t &nat_data) {
  std::stringstream ss;
  for (auto &pyld : nat_data.payload) {
    ss << "nat_event:" << pyld.nat_handle << "|" << pyld.nat_event;
    smi::record::record_add_notify(ss.str());
  }
  if (notification.nat_event) notification.nat_event(nat_data);
  return;
}

void packet_event_notify(const switch_packet_event_data_t &pkt_data) {
  std::stringstream ss;
  ss << "packet_event:" << pkt_data.port_handle << "|"
     << pkt_data.hostif_trap_handle << "|" << pkt_data.pkt_size;
  smi::record::record_add_notify(ss.str());

  if (notification.packet_event) notification.packet_event(pkt_data);
  return;
}

void device_status_notify(const switch_device_event_data_t &device_data) {
  std::stringstream ss;
  ss << "device_status:" << device_data.device_handle << "|"
     << device_data.device_status_event << "|" << device_data.error_type;
  smi::record::record_add_notify(ss.str());

  if (notification.device_event) notification.device_event(device_data);
  return;
}

void bfd_event_notify(const switch_bfd_event_data_t &bfd_data) {
  std::stringstream ss;
  ss << "bfd_event:" << bfd_data.bfd_session_handle << "|"
     << bfd_data.bfd_session_state;
  smi::record::record_add_notify(ss.str());
  if (notification.bfd_event) notification.bfd_event(bfd_data);
  return;
}
}  // namespace event
}  // namespace smi

namespace bf_switch {

switch_status_t bf_switch_event_register(switch_event_t event, void *cb) {
  smi::event::object_event_register(event, cb);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t bf_switch_event_deregister(switch_event_t event) {
  smi::event::object_event_deregister(event);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t bf_switch_object_event_notify_set(
    const switch_object_type_t object_type, const switch_object_event_t event) {
  smi::event::object_event_notify_set(object_type, event);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t bf_switch_object_event_notify_all(
    const switch_object_type_t object_type) {
  smi::event::object_event_notify_set_all(object_type);
  return SWITCH_STATUS_SUCCESS;
}

}  // namespace bf_switch

#ifdef __cplusplus
extern "C" {
#endif

switch_status_t bf_switch_event_register_c(switch_event_t event, void *cb) {
  if (event == SWITCH_MAC_EVENT) {
    smi::event::override_mac_callback_for_c(cb);
    return SWITCH_STATUS_SUCCESS;
  }
  smi::event::object_event_register(event, cb);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t bf_switch_event_deregister_c(switch_event_t event) {
  smi::event::object_event_deregister(event);
  return SWITCH_STATUS_SUCCESS;
}

#ifdef __cplusplus
}
#endif
