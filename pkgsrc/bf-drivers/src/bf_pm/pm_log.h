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


#ifndef __PM_LOG_H__
#define __PM_LOG_H__

#include "target-sys/bf_sal/bf_sys_intf.h"

#define PM_ERROR(format, ...)           \
  bf_sys_log_and_trace(BF_MOD_PM,       \
                       BF_LOG_ERR,      \
                       "%s:%d " format, \
                       __func__,        \
                       __LINE__,        \
                       ##__VA_ARGS__);
#define PM_WARN(format, ...)            \
  bf_sys_log_and_trace(BF_MOD_PM,       \
                       BF_LOG_WARN,     \
                       "%s:%d " format, \
                       __func__,        \
                       __LINE__,        \
                       ##__VA_ARGS__);
#define PM_TRACE(format, ...)           \
  bf_sys_log_and_trace(BF_MOD_PM,       \
                       BF_LOG_INFO,     \
                       "%s:%d " format, \
                       __func__,        \
                       __LINE__,        \
                       ##__VA_ARGS__);
#define PM_DEBUG(format, ...)           \
  bf_sys_log_and_trace(BF_MOD_PM,       \
                       BF_LOG_DBG,      \
                       "%s:%d " format, \
                       __func__,        \
                       __LINE__,        \
                       ##__VA_ARGS__);
#endif /* __PM_LOG_H__ */
