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


/*!
 * @file perf_env.h
 * @date
 *
 * Performance environment handling common definitions.
 */

#ifndef _PERF_ENV_H
#define _PERF_ENV_H

/**
 * @brief Get environment parameters.
 *
 * @param dev_id device id
 * @return env_description
 */
struct env_description environment(bf_dev_id_t dev_id);

#endif
