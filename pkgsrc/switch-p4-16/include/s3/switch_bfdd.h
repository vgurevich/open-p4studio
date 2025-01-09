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


#ifndef INCLUDE_BF_SWITCH_SWITCH_BFDD_H_
#define INCLUDE_BF_SWITCH_SWITCH_BFDD_H_

#include "bf_switch/bf_switch_types.h"
#include "bf_switch/bf_event.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** BFD session types */
typedef enum switch_bfd_session_type_e {
  SWITCH_BFD_ASYNC_ACTIVE,
  SWITCH_BFD_ASYNC_PASSIVE
} switch_bfd_session_type_t;

switch_status_t start_bf_switch_bfdd(uint16_t reason_code);
switch_status_t stop_bf_switch_bfdd(void);

switch_status_t switch_bfdd_session_create(
    // BMAI object handle
    uint64_t bfd_session_handle,
    switch_bfd_session_type_t session_type,
    uint32_t local_discriminator,
    // in microseconds
    uint32_t min_tx,
    // in microseconds
    uint32_t min_rx,
    uint32_t multiplier,
    uint16_t udp_src_port,
    switch_ip_address_t local_ip,
    switch_ip_address_t peer_ip);
switch_status_t switch_bfdd_session_delete(uint32_t local_discriminator,
                                           switch_ip_address_t local_ip,
                                           switch_ip_address_t peer_ip);

typedef struct switch_bfdd_session_params_s {
  uint32_t remote_min_rx;
  uint32_t remote_min_tx;
  uint32_t negotiated_rx;
  uint32_t negotiated_tx;
  uint32_t remote_discriminator;
  uint8_t remote_multiplier;
  uint8_t local_diag;
  uint8_t remote_diag;
  switch_bfd_session_state_t state;
} switch_bfdd_session_params_t;

// BMAI object handle
typedef void (*switch_bfdd_session_state_cb)(
    uint64_t session_id, switch_bfdd_session_params_t params);

switch_status_t switch_register_bfdd_session_state_cb(
    switch_bfdd_session_state_cb cb);

void switch_bfdd_dump_enable(bool);
#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_BF_SWITCH_SWITCH_BFDD_H_
