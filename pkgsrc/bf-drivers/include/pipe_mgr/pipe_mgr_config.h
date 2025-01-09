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
 * @brief pipe_mgr Configuration Header
 *
 * @addtogroup pipe_mgr-config
 * @{
 *
 */
#ifndef __PIPE_MGR_CONFIG_H__
#define __PIPE_MGR_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef PIPE_MGR_INCLUDE_CUSTOM_CONFIG
#include <pipe_mgr_custom_config.h>
#endif

/**
 * PIPE_MGR_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */

#ifndef PIPE_MGR_CONFIG_PORTING_STDLIB
#define PIPE_MGR_CONFIG_PORTING_STDLIB 1
#endif

/**
 * PIPE_MGR_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */

#ifndef PIPE_MGR_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define PIPE_MGR_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS \
  PIPE_MGR_CONFIG_PORTING_STDLIB
#endif

/**
 * PIPE_MGR_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */

#ifndef PIPE_MGR_CONFIG_INCLUDE_UCLI
#define PIPE_MGR_CONFIG_INCLUDE_UCLI 0
#endif

#include "pipe_mgr_porting.h"

#endif /* __PIPE_MGR_CONFIG_H__ */
/* @} */
