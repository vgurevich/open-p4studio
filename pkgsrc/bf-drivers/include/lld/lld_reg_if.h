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

//
//  lld_reg_if.h

#ifndef lld_reg_if_h
#define lld_reg_if_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include "bf_types/bf_types.h"

int lld_write_register(bf_dev_id_t dev_id, uint32_t reg, uint32_t data);
int lld_subdev_write_register(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              uint32_t reg,
                              uint32_t data);

int lld_read_register(bf_dev_id_t dev_id, uint32_t reg, uint32_t *val);
int lld_subdev_read_register(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             uint32_t reg,
                             uint32_t *val);

int lld_ind_write(bf_dev_id_t dev_id,
                  uint64_t ind_addr,
                  uint64_t data_hi,
                  uint64_t data_lo);

uint32_t lld_ind_read(bf_dev_id_t dev_id,
                      uint64_t ind_addr,
                      uint64_t *data_hi,
                      uint64_t *data_lo);
int lld_subdev_ind_write(bf_dev_id_t dev_id,
                         bf_subdev_id_t subdev_id,
                         uint64_t ind_addr,
                         uint64_t data_hi,
                         uint64_t data_lo);

uint32_t lld_subdev_ind_read(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             uint64_t ind_addr,
                             uint64_t *data_hi,
                             uint64_t *data_lo);
void lld_dev_lock_ind_reg_mutex_init(void);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
