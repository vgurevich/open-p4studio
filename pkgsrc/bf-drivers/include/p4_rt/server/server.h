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

#ifndef _P4_RT_SERVER_SERVER_H_
#define _P4_RT_SERVER_SERVER_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

// Start server and bind to default address (0.0.0.0:50051)
void BfP4RtServerRun();

// Start server and bind to given address (eg. localhost:1234,
// 192.168.1.1:31416, [::1]:27182, etc.)
void BfP4RtServerRunAddr(const char *server_address);

// Get port number bound to the server
int BfP4RtServerGetPort();

// Get number of PacketIn packets sent to client
uint64_t BfP4RtServerGetPacketInCount(uint64_t device_id);

// Get number of PacketOut packets sent to DevMgr
uint64_t BfP4RtServerGetPacketOutCount(uint64_t device_id);

// Wait for the server to shutdown. Note that some other thread must be
// responsible for shutting down the server for this call to ever return.
void BfP4RtServerWait();

// Shutdown server but waits for all RPCs to finish
void BfP4RtServerShutdown();

// Force-shutdown server with a deadline for all RPCs to finish
void BfP4RtServerForceShutdown(int deadline_seconds);

// Once server has been shutdown, cleanup allocated resources.
void BfP4RtServerCleanup();

#ifdef __cplusplus
}
#endif

#endif  // _P4_RT_SERVER_SERVER_H_
