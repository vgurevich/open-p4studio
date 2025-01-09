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


#ifndef __COMMON_QOS_PDFIXED_H__
#define __COMMON_QOS_PDFIXED_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <tofino/pdfixed/pd_tm.h>
#ifdef __cplusplus
}
#endif

#include "bf_switch/bf_switch.h"

/** maximum ingress buffer pools */
#define SWITCH_BUFFER_POOL_INGRESS_MAX 4

/** maximum egress buffer pools */
#define SWITCH_BUFFER_POOL_EGRESS_MAX 4

/** maximum buffer profiles */
#define SWITCH_BUFFER_PROFILE_MAX 64

#define SWITCH_BUFFER_PFC_ICOS_MAX 8

#define SWITCH_BUFFER_DYNAMIC_THRESHOLD_FACTOR 32

#define SWITCH_DEFAULT_PPG_INDEX 0xFF

#define SWITCH_MAX_PPGS_PER_PIPE 0x80

/** ASIC's default PPG starts from 128 */
#define SWITCH_ASIC_FIRST_DEFAULT_PPG_HANDLE 128

/** Default buffer pool size */
#define SWITCH_BUFFER_POOL_DEFAULT 80000

#endif  // __COMMON_QOS_PDFIXED_H__
