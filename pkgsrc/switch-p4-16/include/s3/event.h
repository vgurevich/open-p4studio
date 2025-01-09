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


#ifndef INCLUDE_S3_EVENT_H__
#define INCLUDE_S3_EVENT_H__

#include <sstream>

#include "bf_switch/bf_switch_types.h"
#include "bf_switch/bf_event.h"

#ifdef __cplusplus

std::ostream &operator<<(std::ostream &os, const switch_object_event_t &event);

/**
 * @brief Structure containing the various callbacks to register
 */
typedef struct smi_event_cb_s {
  smi_object_event_cb object_event;
  smi_mac_event_cb mac_event;
  smi_mac_event_cb_c mac_event_c;
  smi_nat_event_cb nat_event;
  smi_nat_event_cb_c nat_event_c;
  smi_port_event_cb port_event;
  smi_port_oper_status_event_cb port_oper_status_event;
  smi_packet_event_cb packet_event;
  smi_device_oper_status_event_cb device_event;
  smi_bfd_event_cb bfd_event;
} smi_event_cb_t;

namespace smi {
namespace event {

switch_status_t event_init();
void object_event_register(switch_event_t event, void *cb);
void object_event_deregister(switch_event_t event);
void object_event_notify_set(const switch_object_type_t obj_type,
                             const switch_object_event_t event);
void object_event_notify_set_all(const switch_object_type_t obj_type);
void object_notify(const switch_object_event_t event,
                   const switch_object_type_t object_type,
                   const switch_object_id_t object_id,
                   const switch_attribute_t attr,
                   const switch_status_t status);
void port_status_notify(const switch_port_oper_status_event_t oper_status,
                        const switch_object_id_t port_handle);
void port_event_notify(const switch_port_event_t add,
                       const switch_object_id_t port_handle);
void mac_event_notify(const switch_mac_event_data_t &mac_data);
void nat_event_notify(const switch_nat_event_data_t &nat_data);
void packet_event_notify(const switch_packet_event_data_t &packet_data);
void device_status_notify(const switch_device_event_data_t &device_data);
void bfd_event_notify(const switch_bfd_event_data_t &bfd_data);

} /* namespace event */
} /* namespace smi */

#endif /* __cplusplus */

#endif  // INCLUDE_S3_EVENT_H__
