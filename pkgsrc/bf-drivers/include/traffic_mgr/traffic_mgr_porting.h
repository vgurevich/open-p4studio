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
 * @brief traffic_mgr Porting Macros.
 *
 * @addtogroup traffic_mgr-porting
 * @{
 *
 */
#ifndef __TRAFFIC_MGR_PORTING_H__
#define __TRAFFIC_MGR_PORTING_H__

/* <auto.start.portingmacro(ALL).define> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#include <target-sys/bf_sal/bf_sys_intf.h>

#define TRAFFIC_MGR_MALLOC bf_sys_malloc
#define TRAFFIC_MGR_REALLOC bf_sys_realloc
#define TRAFFIC_MGR_CALLOC bf_sys_calloc
#define TRAFFIC_MGR_FREE bf_sys_free

#define TRAFFIC_MGR_ASSERT bf_sys_assert
#define TRAFFIC_MGR_DBGCHK bf_sys_dbgchk

#ifndef TRAFFIC_MGR_MEMSET
#if defined(GLOBAL_MEMSET)
#define TRAFFIC_MGR_MEMSET GLOBAL_MEMSET
#else
#define TRAFFIC_MGR_MEMSET memset
#endif
#endif

#ifndef TRAFFIC_MGR_MEMCPY
#if defined(GLOBAL_MEMCPY)
#define TRAFFIC_MGR_MEMCPY GLOBAL_MEMCPY
#else
#define TRAFFIC_MGR_MEMCPY memcpy
#endif
#endif

#ifndef TRAFFIC_MGR_STRNCPY
#if defined(GLOBAL_STRNCPY)
#define TRAFFIC_MGR_STRNCPY GLOBAL_STRNCPY
#else
#define TRAFFIC_MGR_STRNCPY strncpy
#endif
#endif

#ifndef TRAFFIC_MGR_VSNPRINTF
#if defined(GLOBAL_VSNPRINTF)
#define TRAFFIC_MGR_VSNPRINTF GLOBAL_VSNPRINTF
#else
#define TRAFFIC_MGR_VSNPRINTF vsnprintf
#endif
#endif

#ifndef TRAFFIC_MGR_SNPRINTF
#if defined(GLOBAL_SNPRINTF)
#define TRAFFIC_MGR_SNPRINTF GLOBAL_SNPRINTF
#else
#define TRAFFIC_MGR_SNPRINTF snprintf
#endif
#endif

#ifndef TRAFFIC_MGR_STRLEN
#if defined(GLOBAL_STRLEN)
#define TRAFFIC_MGR_STRLEN GLOBAL_STRLEN
#else
#define TRAFFIC_MGR_STRLEN strlen
#endif
#endif

/* <auto.end.portingmacro(ALL).define> */

#include <bf_types/bf_types.h>
#define BF_TM_NUM_ASIC BF_MAX_DEV_COUNT       // From bf_types.h
#define BF_TM_NUM_SUBDEV BF_MAX_SUBDEV_COUNT  // From bf_types.h

#ifdef TM_MT_SAFE
#include <target-sys/bf_sal/bf_sys_sem.h>
#include <time.h>
#define tm_mutex_t bf_sys_rmutex_t
#define TM_LOCK(dev, x)                                             \
  do {                                                              \
    if (g_tm_ctx[dev]->internal_call) break;                        \
    int A_A = bf_sys_rmutex_lock(&x);                               \
    if (A_A) {                                                      \
      char errStr[32] = {0};                                        \
      strerror_r(A_A, errStr, 32);                                  \
      LOG_ERROR("LOCK: Error \"%s\" (%d) taking \"%s\" from %s:%d", \
                errStr,                                             \
                A_A,                                                \
                #x,                                                 \
                __func__,                                           \
                __LINE__);                                          \
    }                                                               \
  } while (0);

#define TM_UNLOCK(dev, x)                                                \
  do {                                                                   \
    if (g_tm_ctx[dev]->internal_call) break;                             \
    int A_A = bf_sys_rmutex_unlock(&x);                                  \
    if (A_A) {                                                           \
      char errStr[32] = {0};                                             \
      strerror_r(A_A, errStr, 32);                                       \
      LOG_ERROR("UNLOCK: Error \"%s\" (%d) releasing \"%s\" from %s:%d", \
                errStr,                                                  \
                A_A,                                                     \
                #x,                                                      \
                __func__,                                                \
                __LINE__);                                               \
    }                                                                    \
  } while (0);

#define TM_LOCK_INIT(x)       \
  do {                        \
    bf_sys_rmutex_init(&(x)); \
  } while (0);

#define TM_MUTEX_LOCK(x)                                            \
  do {                                                              \
    int A_A = bf_sys_rmutex_lock(x);                                \
    if (A_A) {                                                      \
      char errStr[32] = {0};                                        \
      strerror_r(A_A, errStr, 32);                                  \
      LOG_ERROR("LOCK: Error \"%s\" (%d) taking \"%s\" from %s:%d", \
                errStr,                                             \
                A_A,                                                \
                #x,                                                 \
                __func__,                                           \
                __LINE__);                                          \
    }                                                               \
  } while (0);

#define TM_MUTEX_UNLOCK(x)                                               \
  do {                                                                   \
    int A_A = bf_sys_rmutex_unlock(x);                                   \
    if (A_A) {                                                           \
      char errStr[32] = {0};                                             \
      strerror_r(A_A, errStr, 32);                                       \
      LOG_ERROR("UNLOCK: Error \"%s\" (%d) releasing \"%s\" from %s:%d", \
                errStr,                                                  \
                A_A,                                                     \
                #x,                                                      \
                __func__,                                                \
                __LINE__);                                               \
    }                                                                    \
  } while (0);
#else
#define tm_mutex_t int
#define TM_LOCK(dev, x)
#define TM_UNLOCK(dev, x)
#define TM_LOCK_INIT(x)
#define TM_MUTEX_LOCK(x)
#define TM_MUTEX_UNLOCK(x)
#endif

#endif /* __TRAFFIC_MGR_PORTING_H__ */
       /* @} */
