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


#include <errno.h>
#include <time.h>
#include <math.h>

#include <bfutils/bf_utils.h>

#include <target-utils/uCli/ucli.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_dev.h>
#include <pipe_mgr/pipe_mgr_drv.h>
#include <tofino_regs/tofino.h>

#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>
#include <lld/tof2_reg_drv_defs.h>
#include <lld/tofino_defs.h>

#include "perf_util.h"
#include <perf/perf_common_intf.h>
#include <perf/perf_env_intf.h>
#include "perf_env.h"

struct env_description env_desc = {
    .status = false,
    .num_params = ENV_PARAM_N_MAX,
    .param = {{.name = "Family"},
              {.name = "Type"},
              {.name = "SKU"},
              {.name = "bf-syslibs"},
              {.name = "bf-syslibs (internal)"},
              {.name = "bf-utils"},
              {.name = "bf-utils (internal)"},
              {.name = "bf-drivers"},
              {.name = "bf-drivers (internal)"}}};

/**
 * @brief Set environment parameter.
 *
 * @param char* dest
 * @param char* src
 * @return bool
 */
bool set_env_param(char *dest, const char *src) {
  if (!dest || !src) {
    bf_sys_dbgchk(0);
    return false;
  }

  int len = strlen(src) > ENV_PARAM_VALUE_L_MAX - 1 ? ENV_PARAM_VALUE_L_MAX - 1
                                                    : strlen(src);
  memcpy(dest, src, len);
  dest[len] = '\0';

  return true;
}

/**
 * @brief Get environment parameters.
 *
 * @param dev_id device id
 * @return env_description
 */
struct env_description environment(bf_dev_id_t dev_id) {
  env_desc.status = false;

  bf_dev_type_t dev_type = lld_dev_type_get(dev_id);
  bf_dev_family_t family = bf_dev_type_to_family(dev_type);

  bool is_sw_model;
  bf_status_t sts = bf_drv_device_type_get(dev_id, &is_sw_model);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d: Error getting device type for device: %d\n",
              __func__,
              __LINE__,
              dev_id);
    bf_sys_dbgchk(0);
    return env_desc;
  }

  if (!set_env_param(env_desc.param[ENV_FAMILY].value,
                     bf_dev_family_str(family)) ||
      !set_env_param(env_desc.param[ENV_TYPE].value,
                     is_sw_model ? "model" : "HW") ||
      !set_env_param(env_desc.param[ENV_SKU].value,
                     lld_sku_get_sku_name(dev_id)) ||
      !set_env_param(env_desc.param[ENV_SYSLIBS].value,
                     target_syslib_get_version()) ||
      !set_env_param(env_desc.param[ENV_SYSLIBS_INT].value,
                     target_syslib_get_internal_version()) ||
      !set_env_param(env_desc.param[ENV_UTILS].value, bf_utils_get_version()) ||
      !set_env_param(env_desc.param[ENV_UTILS_INT].value,
                     bf_utils_get_internal_version()) ||
      !set_env_param(env_desc.param[ENV_DRIVERS].value, bf_drv_get_version()) ||
      !set_env_param(env_desc.param[ENV_DRIVERS_INT].value,
                     bf_drv_get_internal_version())) {
    LOG_ERROR(
        "%s:%d: Error setting environment parameter\n", __func__, __LINE__);
    return env_desc;
  }

  env_desc.status = true;
  return env_desc;
}
