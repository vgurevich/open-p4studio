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


/*
 * C/C++ header file for calling server start function from C code
 */

#ifndef _DIAG_RPC_SERVER_H_
#define _DIAG_RPC_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DIAG_RPC_SERVER_PORT 9096

extern int start_diag_rpc_server(void **server_cookie);
extern int stop_diag_rpc_server();

#ifdef __cplusplus
}
#endif

#endif /* _DIAG_RPC_SERVER_H_ */
