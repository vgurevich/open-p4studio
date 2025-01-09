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


/*
 * This file contains error codes
 */

#ifndef __TM_ERROR_H__
#define __TM_ERROR_H__

#include <stdint.h>
#include <stdbool.h>
#include <bf_types/bf_types.h>

typedef enum {
  BF_TM_EOK = BF_SUCCESS,
  BF_TM_EINT = BF_INTERNAL_ERROR,
  BF_TM_EINV_ARG = BF_INVALID_ARG,
} bf_tm_error_en;

#define BF_TM_IS_OK(rc) (rc == BF_TM_EOK)
#define BF_TM_IS_NOTOK(rc) (rc != BF_TM_EOK)

#endif
