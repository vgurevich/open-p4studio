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


#ifndef INCLUDE_BF_SWITCH_BF_EVENT_H__
#define INCLUDE_BF_SWITCH_BF_EVENT_H__

#include "bf_switch_types.h"

/** @defgroup event Event notifications API
 *  API functions, enums and structures to register and manage notifications
 *  @{
 */

/**
 * @brief Set of possible object events
 */
typedef enum switch_object_event_s {
  SWITCH_OBJECT_EVENT_NONE = (1 << 0),
  SWITCH_OBJECT_EVENT_CREATE = (1 << 1),
  SWITCH_OBJECT_EVENT_DELETE = (1 << 2),
  SWITCH_OBJECT_EVENT_SET = (1 << 3),
  SWITCH_OBJECT_EVENT_GET = (1 << 4)
} switch_object_event_t;

/**
 * @brief Structure containing the object event data
 */
typedef struct switch_object_event_data_s {
  switch_object_event_t event;
  switch_object_type_t object_type;
  switch_object_id_t object_id;
  switch_attribute_t attr;
  switch_status_t status;
} switch_object_event_data_t;

/**
 * @brief Set of possible object events
 */
typedef enum switch_mac_event_s {
  SWITCH_MAC_EVENT_LEARN,
  SWITCH_MAC_EVENT_AGE,
  SWITCH_MAC_EVENT_MOVE,
  SWITCH_MAC_EVENT_DELETE,
  SWITCH_MAC_EVENT_CREATE,
} switch_mac_event_t;

/**
 * @brief Structure containing the MAC payload with the mac handle and event
 * type
 */
typedef struct switch_mac_payload_t {
  switch_mac_event_t mac_event;
  switch_object_id_t mac_handle;
  switch_object_id_t port_lag_handle;
} switch_mac_payload_t;

/**
 * @brief Set of possible object events
 */
typedef enum switch_nat_event_s {
  SWITCH_NAT_EVENT_NONE,
  SWITCH_NAT_EVENT_AGED,
} switch_nat_event_t;

/**
 * @brief Structure containing the nat payload with the nat handle and event
 * type
 */
typedef struct switch_nat_payload_t {
  switch_nat_event_t nat_event;
  switch_object_id_t nat_handle;
} switch_nat_payload_t;

/**
 * @brief Set of bfd session states
 */
typedef enum switch_bfd_session_state_s {
  SWITCH_BFD_SESSION_STATE_ADMIN_DOWN,
  SWITCH_BFD_SESSION_STATE_DOWN,
  SWITCH_BFD_SESSION_STATE_INIT,
  SWITCH_BFD_SESSION_STATE_UP,
} switch_bfd_session_state_t;

// BFD event payload
/**
 * @brief Structure containing the bfd payload with state and session handle
 */
typedef struct switch_bfd_event_data_s {
  switch_bfd_session_state_t bfd_session_state;
  switch_object_id_t bfd_session_handle;
} switch_bfd_event_data_t;

/**
 * @brief Set of possible port events
 */
typedef enum switch_port_event_s {
  SWITCH_PORT_EVENT_NONE = (1 << 0),
  SWITCH_PORT_EVENT_ADD = (1 << 1),
  SWITCH_PORT_EVENT_DELETE = (1 << 2)
} switch_port_event_t;

/**
 * @brief Structure containing the port event data
 */
typedef struct switch_port_event_data_s {
  switch_port_event_t port_event;
  switch_object_id_t object_id;
} switch_port_event_data_t;

/**
 * @brief Set of possible port operational state change events
 */
typedef enum switch_port_oper_status_event_s {
  SWITCH_PORT_OPER_STATUS_UNKNOWN,
  SWITCH_PORT_OPER_STATUS_UP,
  SWITCH_PORT_OPER_STATUS_DOWN
} switch_port_oper_status_event_t;

/**
 * @brief Structure containing the port operation state change event data
 */
typedef struct switch_port_oper_status_event_data_s {
  switch_port_oper_status_event_t port_status_event;
  switch_object_id_t object_id;
} switch_port_oper_status_event_data_t;

/**
 * @brief Structure containing the packet event data
 */
typedef struct switch_packet_event_data_s {
  switch_object_id_t port_handle;
  switch_object_id_t hostif_trap_handle;
  char *pkt;
  int pkt_size;
} switch_packet_event_data_t;

/**
 * @brief Set of possible device operational state change events
 */
typedef enum switch_device_oper_status_event_s {
  SWITCH_DEVICE_OPER_STATUS_UNKNOWN,
  SWITCH_DEVICE_OPER_STATUS_UP,
  SWITCH_DEVICE_OPER_STATUS_DOWN,
  SWITCH_DEVICE_OPER_STATUS_FAILED
} switch_device_oper_status_event_t;

/**
 * @brief Set of possible device error types
 */
typedef enum switch_device_error_type_s {
  SWITCH_DEVICE_ERROR_TYPE_GENERIC = 0,
  SWITCH_DEVICE_ERROR_TYPE_SINGLE_BIT_ECC,
  SWITCH_DEVICE_ERROR_TYPE_MULTI_BIT_ECC,
  SWITCH_DEVICE_ERROR_TYPE_PARITY,
  SWITCH_DEVICE_ERROR_TYPE_OVERFLOW,
  SWITCH_DEVICE_ERROR_TYPE_UNDERFLOW,
  SWITCH_DEVICE_ERROR_TYPE_PKT_DROP
} switch_device_error_type_t;

/**
 * @brief Structure containing the device event data
 */
typedef struct switch_device_event_data_s {
  uint32_t device_handle;
  switch_device_oper_status_event_t device_status_event;
  switch_device_error_type_t error_type;
} switch_device_event_data_t;

/**
 * @brief Object events
 *
 * @param[in] data - object event data
 */
typedef void (*smi_object_event_cb)(const switch_object_event_data_t data);

/**
 * @brief Port events
 *
 * @param[in] data - Port event data
 */
typedef void (*smi_port_event_cb)(const switch_port_event_data_t data);

/**
 * @brief Port oper status change events
 *
 * @param[in] data - Port oper status event data paylaod
 */
typedef void (*smi_port_oper_status_event_cb)(
    const switch_port_oper_status_event_data_t data);

/**
 * @brief Packet event
 *
 * @param[in] data - Packet event data paylaod
 */
typedef void (*smi_packet_event_cb)(const switch_packet_event_data_t data);

/**
 * @brief Device oper status events
 *
 * @param[in] data - Device oper status event data paylaod
 */
typedef void (*smi_device_oper_status_event_cb)(
    const switch_device_event_data_t data);

/**
 * @brief Set of possible events to register for
 */
typedef enum switch_event_s {
  SWITCH_OBJECT_EVENT,
  SWITCH_MAC_EVENT,
  SWITCH_NAT_EVENT,
  SWITCH_PORT_EVENT,
  SWITCH_PORT_OPER_STATUS_EVENT,
  SWITCH_PACKET_EVENT,
  SWITCH_DEVICE_EVENT,
  SWITCH_BFD_EVENT
} switch_event_t;

#ifdef __cplusplus
#include <vector>

/**
 * @brief Structure containing the MAC event data
 */
typedef struct switch_mac_event_data_s {
  std::vector<switch_mac_payload_t> payload;
} switch_mac_event_data_t;

/**
 * @brief MAC events
 *
 * @param[in] data - MAC event data
 */
typedef void (*smi_mac_event_cb)(const switch_mac_event_data_t &data);

/**
 * @brief Structure containing the nat event data
 */
typedef struct switch_nat_event_data_s {
  std::vector<switch_nat_payload_t> payload;
} switch_nat_event_data_t;

/**
 * @brief nat events
 *
 * @param[in] data - nat event data
 */
typedef void (*smi_nat_event_cb)(const switch_nat_event_data_t &data);

/**
 * @brief bfd events
 *
 * @param[in] data - bfd event data
 */
typedef void (*smi_bfd_event_cb)(const switch_bfd_event_data_t &data);

namespace bf_switch {

/**
 * @brief Register for callback on events
 *
 * @param[in] event - Event type to register
 * @param[in] cb - Pointer to the callback
 */
switch_status_t bf_switch_event_register(switch_event_t event, void *cb);

/**
 * @brief Deregister from callbacks
 *
 * @param[in] event - Event type to deregister
 */
switch_status_t bf_switch_event_deregister(switch_event_t event);

/**
 * @brief Object per event notify settings
 *
 * @param[in] object_type - object type to nofify
 * @param[in] event - event type notification setting
 */
switch_status_t bf_switch_object_event_notify_set(
    const switch_object_type_t object_type, const switch_object_event_t event);

/**
 * @brief Notify for all object events
 *
 * @param[in] object_type - object type to nofify
 */
switch_status_t bf_switch_object_event_notify_all(
    const switch_object_type_t object_type);

}  // namespace bf_switch

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure containing the MAC event data
 */
typedef struct switch_mac_event_data_c_s {
  switch_mac_payload_t *payload;
  uint32_t count;
} switch_mac_event_data_c_t;

/**
 * @brief MAC events
 * \n C wrapper for \ref smi_mac_event_cb
 * The memory pointed to by data->payload has to be free'd in the callback
 * The memory pointed to by data has to be free'd in the callback
 *
 * @param[in] data - MAC event data
 */
typedef void (*smi_mac_event_cb_c)(switch_mac_event_data_c_t *data);

/**
 * @brief Structure containing the nat event data
 */
typedef struct switch_nat_event_data_c_s {
  switch_nat_payload_t *payload;
  uint32_t count;
} switch_nat_event_data_c_t;

/**
 * @brief nat events
 * \n C wrapper for \ref smi_nat_event_cb
 * The memory pointed to by data->payload has to be free'd in the callback
 * The memory pointed to by data has to be free'd in the callback
 *
 * @param[in] data - nat event data
 */
typedef void (*smi_nat_event_cb_c)(switch_nat_event_data_c_t *data);

/**
 * @brief Register for callback on events
 * \n C wrapper for \ref bf_switch_event_register
 *
 * @param[in] event - Event type to register
 * @param[in] cb - Pointer to the callback
 */
switch_status_t bf_switch_event_register_c(switch_event_t event, void *cb);

/**
 * @brief Deregister from callbacks
 * \n C wrapper for \ref bf_switch_event_deregister
 *
 * @param[in] event - Event type to deregister
 */
switch_status_t bf_switch_event_deregister_c(switch_event_t event);

#ifdef __cplusplus
}
#endif

/** @} */

#endif  // INCLUDE_BF_SWITCH_BF_EVENT_H__
