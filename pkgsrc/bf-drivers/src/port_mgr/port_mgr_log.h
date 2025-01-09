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


#ifndef PORT_MGR_LOG_INCLUDED
#define PORT_MGR_LOG_INCLUDED

#include <target-sys/bf_sal/bf_sys_intf.h>

#define port_mgr_log_critical(...) \
  bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_CRIT, __VA_ARGS__)
#define port_mgr_log_error(...) \
  bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_ERR, __VA_ARGS__)
#define port_mgr_log_warn(...) \
  bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_WARN, __VA_ARGS__)
#define port_mgr_log_trace(...) \
  bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_INFO, __VA_ARGS__)
#define port_mgr_log_debug(...) \
  bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_DBG, __VA_ARGS__)

#define port_mgr_log port_mgr_log_debug

void port_mgr_log_internal(const char *fmt, ...);

extern void port_mgr_submodule_debug_set(int submodule);
extern int is_port_mgr_submodule_log_enable(int submodule_bitmap);

#define port_mgr_submodule_log(submodule_bitmap, ...)               \
  do {                                                              \
    if (is_port_mgr_submodule_log_enable(submodule_bitmap))         \
      bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_DBG, ##__VA_ARGS__); \
  } while (0)

#define PORT_MGR_SUBMOD_INTR (1 << PORT_MGR_SUBMODULE_INTR)
/*
We can have at max 32 submodules i.e. 0 to 31
*/
typedef enum {
  PORT_MGR_SUBMODULE_INTR = 0,
  PORT_MGR_SUBMODULE_MAX = 32
} port_mgr_submodule_t;

#endif  // PORT_MGR_LOG_INCUDED
