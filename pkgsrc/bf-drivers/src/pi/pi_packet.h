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


#ifndef _PI_PACKET_H__
#define _PI_PACKET_H__

#include <PI/pi_base.h>

pi_status_t packet_register_with_pkt_mgr(pi_dev_id_t dev_id);

pi_status_t packet_send_to_pkt_mgr(pi_dev_id_t dev_id,
                                   const char *out_packet,
                                   size_t packet_size);

#endif  // _PI_PACKET_H__
