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


#ifndef __PIPE_MGR_LOG_H__
#define __PIPE_MGR_LOG_H__

#include <target-sys/bf_sal/bf_sys_intf.h>

#define LOG_CRIT(...) \
  bf_sys_log_and_trace(BF_MOD_PIPE, BF_LOG_CRIT, __VA_ARGS__)
#define LOG_ERROR(...) \
  bf_sys_log_and_trace(BF_MOD_PIPE, BF_LOG_ERR, __VA_ARGS__)
#define LOG_WARN(...) \
  bf_sys_log_and_trace(BF_MOD_PIPE, BF_LOG_WARN, __VA_ARGS__)
#define LOG_TRACE(...) \
  bf_sys_log_and_trace(BF_MOD_PIPE, BF_LOG_INFO, __VA_ARGS__)
#define LOG_DBG(...) bf_sys_log_and_trace(BF_MOD_PIPE, BF_LOG_DBG, __VA_ARGS__)

#define PIPE_MGR_DBG_ALL 0xFFFFFFFF

/*
 * We can have at max 32 submodules, 0 to 31
 */
typedef enum {
  PIPE_MGR_LOG_DBG_ACTION = 0,
  PIPE_MGR_LOG_DBG_ADT,
  PIPE_MGR_LOG_DBG_ALPM,
  PIPE_MGR_LOG_DBG_EXM,
  PIPE_MGR_LOG_DBG_IDLE,
  PIPE_MGR_LOG_DBG_INTERRUPT,
  PIPE_MGR_LOG_DBG_METER,
  PIPE_MGR_LOG_DBG_MIRROR,
  PIPE_MGR_LOG_DBG_PKTGEN,
  PIPE_MGR_LOG_DBG_SELECTOR,
  PIPE_MGR_LOG_DBG_SESSION,
  PIPE_MGR_LOG_DBG_STAT,
  PIPE_MGR_LOG_DBG_STFUL,
  PIPE_MGR_LOG_DBG_TCAM,
  PIPE_MGR_LOG_DBG_ALL = 32
} pipe_mgr_submodule_t;

#define PIPE_MGR_LOG_DBG(submodule_bit, ...)                        \
  do {                                                              \
    if (pipe_mgr_submodule_debug_get() & submodule_bit)             \
      bf_sys_log_and_trace(BF_MOD_PIPE, BF_LOG_DBG, ##__VA_ARGS__); \
  } while (0)

uint32_t pipe_mgr_submodule_debug_get(void);
pipe_status_t pipe_mgr_submodule_debug_set(pipe_mgr_submodule_t submodule);
pipe_status_t pipe_mgr_submodule_debug_reset(pipe_mgr_submodule_t submodule);

#endif /* __PIPE_MGR_LOG_H__ */
