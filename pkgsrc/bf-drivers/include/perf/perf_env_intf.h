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
 * @file perf_env_intf.h
 * @date
 *
 * Environment handling definitions.
 */

#ifndef _PERF_ENV_INTF_H
#define _PERF_ENV_INTF_H

enum env_enum_param {
  ENV_FAMILY,
  ENV_TYPE,
  ENV_SKU,
  ENV_SYSLIBS,
  ENV_SYSLIBS_INT,
  ENV_UTILS,
  ENV_UTILS_INT,
  ENV_DRIVERS,
  ENV_DRIVERS_INT,
  ENV_PARAM_N_MAX
};

struct env_param {
  char *name;
  char value[ENV_PARAM_VALUE_L_MAX];
};

struct env_description {
  bool status;
  int num_params;
  struct env_param param[ENV_PARAM_N_MAX];
};

/**
 * @brief Get environment parameters.
 *
 * @param dev_id device id
 * @return env_description
 */
struct env_description environment(bf_dev_id_t dev_id);

#endif
