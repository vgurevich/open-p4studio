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


/**
 *
 * @file
 * @brief pipe_mgr Porting Macros.
 *
 * @addtogroup pipe_mgr-porting
 * @{
 *
 */
#ifndef __PIPE_MGR_PORTING_H__
#define __PIPE_MGR_PORTING_H__

#include <pipe_mgr/pipe_mgr_config.h>

/* <auto.start.portingmacro(ALL).define> */
#if PIPE_MGR_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#include <assert.h>
#endif
#include <target-sys/bf_sal/bf_sys_intf.h>

#define PIPE_MGR_MALLOC bf_sys_malloc
#define PIPE_MGR_REALLOC bf_sys_realloc
#define PIPE_MGR_CALLOC bf_sys_calloc
#define PIPE_MGR_FREE bf_sys_free

#define PIPE_MGR_ASSERT bf_sys_assert
#define PIPE_MGR_DBGCHK bf_sys_dbgchk

#ifndef PIPE_MGR_MEMSET
#if defined(GLOBAL_MEMSET)
#define PIPE_MGR_MEMSET GLOBAL_MEMSET
#elif PIPE_MGR_CONFIG_PORTING_STDLIB == 1
#define PIPE_MGR_MEMSET memset
#else
#error The macro PIPE_MGR_MEMSET is required but cannot be defined.
#endif
#endif

#ifndef PIPE_MGR_MEMCPY
#if defined(GLOBAL_MEMCPY)
#define PIPE_MGR_MEMCPY GLOBAL_MEMCPY
#elif PIPE_MGR_CONFIG_PORTING_STDLIB == 1
#define PIPE_MGR_MEMCPY memcpy
#else
#error The macro PIPE_MGR_MEMCPY is required but cannot be defined.
#endif
#endif

#ifndef PIPE_MGR_MEMCMP
#if defined(GLOBAL_MEMCMP)
#define PIPE_MGR_MEMCMP GLOBAL_MEMCMP
#elif PIPE_MGR_CONFIG_PORTING_STDLIB == 1
#define PIPE_MGR_MEMCMP memcmp
#else
#error The macro PIPE_MGR_MEMCMP is required but cannot be defined.
#endif
#endif

/* <auto.end.portingmacro(ALL).define> */

#define PIPE_MGR_NUM_DEVICES BF_MAX_DEV_COUNT
#define PIPE_MGR_NUM_SUBDEVICES BF_MAX_SUBDEV_COUNT
#define PIPE_MGR_MAX_SESSIONS 20

/* Keeping the below defines decoupled from Tofino. These just define the
 * max values possible. Used for array declarations etc. For the
 * exact number of pipes in a device, the dev_info structure needs to be
 * looked up
 */
#define PIPE_MGR_MAX_PIPES BF_PIPE_COUNT

#endif /* __PIPE_MGR_PORTING_H__ */
/* @} */
