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


#ifndef port_mgr_memory_mapping_h
#define port_mgr_memory_mapping_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/* Macros to convert and address to/from various address space
 *  perspectives.
 */
#define port_mgr_make_virtual(u64) ((void *)((uintptr_t)u64))
#define port_mgr_make_wd_sz(ptr) ((uintptr_t)ptr)

#define VIRTUAL(x) port_mgr_make_virtual(x)
#define PHYSICAL(x) port_mgr_make_wd_sz(x)

#ifdef __cplusplus
}
#endif /* C++ */

#endif
