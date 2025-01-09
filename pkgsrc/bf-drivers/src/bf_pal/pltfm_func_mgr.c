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

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <tofino/bf_pal/pltfm_func_mgr.h>

pltfm_func_s pltfm_func;

bf_status_t bf_pal_pltfm_fn_reg(pltfm_func_s *param) {
  if (param == NULL) {
    return BF_INVALID_ARG;
  }

  memcpy((char *)&pltfm_func, (char *)param, sizeof(pltfm_func_s));
  return BF_SUCCESS;
}

bf_status_t bf_pal_pltfm_fn_get(pltfm_func_s *ptr) {
  if (ptr == NULL) {
    return BF_INVALID_ARG;
  }

  memcpy((char *)ptr, (char *)&pltfm_func, sizeof(pltfm_func_s));
  return BF_SUCCESS;
}

bf_status_t bf_pal_pltfm_type_get_fn_get(
    bf_pal_pltfm_type_get_fn *pltfm_type_get_fn) {
  if (pltfm_func.pltfm_type_get == NULL) {
    return BF_NOT_IMPLEMENTED;
  }

  *pltfm_type_get_fn = pltfm_func.pltfm_type_get;

  return BF_SUCCESS;
}
