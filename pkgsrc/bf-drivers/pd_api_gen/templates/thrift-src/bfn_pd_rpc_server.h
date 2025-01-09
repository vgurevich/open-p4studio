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

#ifndef _BFN_PD_RPC_SERVER_H_
#define _BFN_PD_RPC_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int add_to_rpc_server(void *processor);
extern int rmv_from_rpc_server(void *processor);

#ifdef __cplusplus
}
#endif



#endif /* _BFN_PD_RPC_SERVER_H_ */
