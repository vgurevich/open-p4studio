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


#ifndef LLD_INTERRUPT_INCLUDED
#define LLD_INTERRUPT_INCLUDED

#include <lld/lld_int_cb.h>

typedef bf_int_cb lld_int_cb;
typedef bf_status_t (*lld_blk_int_traverse_cb)(bf_dev_id_t dev_id,
                                               bf_subdev_id_t subdev_id,
                                               void *blk_lvl_int);

#define LLD_MAX_INT_NBR (511 /* For both Tofino and Tof2, 0-511*/)
#define LLD_TOF_TOF2_TOF3_SHADOW_REG_NUMB \
  (16) /* For both Tofino and Tof2, 0-15 */

#endif  // LLD_INTERRUPT_INCUDED
